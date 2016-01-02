/*
 * Copyright (c) 2015, Devan Lai
 *
 * Permission to use, copy, modify, and/or distribute this software
 * for any purpose with or without fee is hereby granted, provided
 * that the above copyright notice and this permission notice
 * appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 * AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <stdio.h>
#include <string.h>

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/crs.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/desig.h>
#include <libopencm3/stm32/iwdg.h>

#include "USB/composite_usb_conf.h"
#include "USB/cdc.h"
#include "USB/mtp.h"
#include "USB/dfu.h"

#include "DAP/app.h"
#include "DAP/CMSIS_DAP_config.h"
#include "DFU/DFU.h"

#include "tick.h"
#include "retarget.h"
#include "console.h"

void led_num(uint8_t value);
void led_bit(uint8_t position, bool state);

#define DEFAULT_BAUDRATE 115200

/* Set STM32 to 48 MHz. */
static void clock_setup(void) {
    rcc_clock_setup_in_hsi48_out_48mhz();

    // Trim from USB sync frame
    crs_autotrim_usb_enable();
    rcc_set_usbclk_source(RCC_HSI48);
}

static inline uint32_t millis(void) {
    return get_ticks();
}

static inline void wait_ms(uint32_t duration_ms) {
    uint32_t now = millis();
    uint32_t end = now + duration_ms;
    if (end < now) {
        end = 0xFFFFFFFFU - end;
        while (millis() >= now) {
            __asm__("NOP");
        }
    }

    while (millis() < end) {
        __asm__("NOP");
    }
}

static void gpio_setup(void) {
    /*
      Button on PB8
      LED0, 1, 2 on PA0, PA1, PA4
      TX, RX (MCU-side) on PA2, PA3
      TGT_RST on PB1
      TGT_SWDIO, TGT_SWCLK on PA5, PA6
      TGT_SWO on PA7
    */

    /* Enable GPIOA and GPIOB clocks. */
    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_GPIOB);


    /* Setup LEDs as open-drain outputs */
    gpio_set_output_options(GPIOA, GPIO_OTYPE_OD, GPIO_OSPEED_LOW,
                            GPIO0 | GPIO1 | GPIO4);

    gpio_mode_setup(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE,
                    GPIO0 | GPIO1 | GPIO4);
}

static void button_setup(void) {
    /* Enable GPIOB clock. */
    rcc_periph_clock_enable(RCC_GPIOB);

    /* Set PB8 to an input */
    gpio_mode_setup(GPIOB, GPIO_MODE_INPUT, GPIO_PUPD_NONE, GPIO8);
}

void led_bit(uint8_t position, bool state) {
    uint32_t gpio = 0xFFFFFFFFU;
    if (position == 0) {
        gpio = GPIO4;
    } else if (position == 1) {
        gpio = GPIO1;
    } else if (position == 2) {
        gpio = GPIO0;
    }

    if (gpio != 0xFFFFFFFFU) {
        if (state) {
            gpio_clear(GPIOA, gpio);
        } else {
            gpio_set(GPIOA, gpio);
        }
    }
}

void led_num(uint8_t value) {
    if (value & 0x4) {
        gpio_clear(GPIOA, GPIO0);
    } else {
        gpio_set(GPIOA, GPIO0);
    }
    if (value & 0x2) {
        gpio_clear(GPIOA, GPIO1);
    } else {
        gpio_set(GPIOA, GPIO1);
    }
    if (value & 0x1) {
        gpio_clear(GPIOA, GPIO4);
    } else {
        gpio_set(GPIOA, GPIO4);
    }
}

static uint32_t usb_timer = 0;

static void on_host_tx(uint8_t* data, uint16_t len) {
    usb_timer = 1000;

    uint16_t i;
    for (i=0; i < len; i++) {
        char c = (char)(data[0]);
        if (c >= 'a' && c <= 'z') {
            cdc_putchar(c - ('a' - 'A'));
        } else if (c >= 'A' && c <= 'Z') {
            cdc_putchar(c + ('a' - 'A'));
        } else if (c == '\r') {
            cdc_putchar('\r');
            cdc_putchar('\n');
        }
    }
}

static void on_host_rx(uint8_t* data, uint16_t* len) {
    usb_timer = 1000;
    *len = 0;
    //*len = (uint16_t)console_recv_buffered(data, USB_CDC_MAX_PACKET_SIZE);
}

static struct usb_cdc_line_coding current_line_coding = {
    .dwDTERate = DEFAULT_BAUDRATE,
    .bCharFormat = USB_CDC_1_STOP_BITS,
    .bParityType = USB_CDC_NO_PARITY,
    .bDataBits = 8
};

static bool on_set_line_coding(const struct usb_cdc_line_coding* line_coding) {
    uint32_t databits;
    if (line_coding->bDataBits == 7 || line_coding->bDataBits == 8) {
        databits = line_coding->bDataBits;
    } else if (line_coding->bDataBits == 0) {
        // Work-around for PuTTY on Windows
        databits = current_line_coding.bDataBits;
    } else {
        return false;
    }

    uint32_t stopbits;
    switch (line_coding->bCharFormat) {
        case USB_CDC_1_STOP_BITS:
            stopbits = USART_STOPBITS_1;
            break;
        case USB_CDC_2_STOP_BITS:
            stopbits = USART_STOPBITS_2;
            break;
        default:
            return false;
    }

    uint32_t parity;
    switch(line_coding->bParityType) {
        case USB_CDC_NO_PARITY:
            parity = USART_PARITY_NONE;
            break;
        case USB_CDC_ODD_PARITY:
            parity = USART_PARITY_ODD;
            break;
        case USB_CDC_EVEN_PARITY:
            parity = USART_PARITY_EVEN;
            break;
        default:
            return false;
    }

    console_reconfigure(line_coding->dwDTERate, databits, stopbits, parity);
    memcpy(&current_line_coding, (const void*)line_coding, sizeof(current_line_coding));

    if (line_coding->bDataBits == 0) {
        current_line_coding.bDataBits = databits;
    }
    return true;
}

static bool on_get_line_coding(struct usb_cdc_line_coding* line_coding) {
    memcpy(line_coding, (const void*)&current_line_coding, sizeof(current_line_coding));
    return true;
}

static void on_mtp_recv(uint8_t* data, uint16_t len) {
    usb_timer = 1000;
}

static void on_mtp_send(uint8_t*data, uint16_t *len) {
    usb_timer = 1000;
    *len = 0;
}

static bool do_reset_to_dfu = false;
static void on_dfu_request(void) {
    do_reset_to_dfu = true;
}

int main(void) {
    DFU_maybe_jump_to_bootloader();

    clock_setup();
    tick_setup(1000);
    button_setup();
    gpio_setup();
    led_num(0);

    console_setup(DEFAULT_BAUDRATE);
    retarget(STDOUT_FILENO, USB_SERIAL);
    retarget(STDERR_FILENO, USB_SERIAL);

    led_num(1);

    {
        char serial[USB_SERIAL_NUM_LENGTH+1];
        desig_get_unique_id_as_string(serial, USB_SERIAL_NUM_LENGTH+1);
        cmp_set_usb_serial_number(serial);
    }

    usbd_device* usbd_dev = cmp_usb_setup();
    DAP_app_setup(usbd_dev, &on_dfu_request);
    cdc_setup(usbd_dev, &on_host_rx, &on_host_tx,
              NULL, &on_set_line_coding, &on_get_line_coding);
    mtp_setup(usbd_dev, &on_mtp_recv, &on_mtp_send);
    dfu_setup(usbd_dev, &on_dfu_request);

    tick_start();


    /* Enable the watchdog to enable DFU recovery from bad firmware images */
    iwdg_set_period_ms(1000);
    iwdg_start();

    while (1) {
        iwdg_reset();
        usbd_poll(usbd_dev);

        // Handle CDC
        bool cdc_active = cdc_update();
        if (cdc_active) {
            usb_timer = 1000;
        }

        // Handle DAP
        bool dap_active = DAP_app_update();
        if (dap_active) {
            usb_timer = 1000;
        } else if (do_reset_to_dfu) {
            /* Blink 3 times to indicate reset */
            int x;
            for (x=0; x < 3; x++) {
                led_num(7);
                wait_ms(150);
                led_num(0);
                wait_ms(150);
            }

            DFU_reset_and_jump_to_bootloader();
        }

        if (usb_timer > 0) {
            usb_timer--;
            LED_ACTIVITY_OUT(1);
        } else {
            LED_ACTIVITY_OUT(0);
        }
    }

    return 0;
}
