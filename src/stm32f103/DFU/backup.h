/*
 * Copyright (c) 2016, Devan Lai
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

#ifndef BACKUP_H_INCLUDED
#define BACKUP_H_INCLUDED

#include <libopencm3/stm32/rtc.h>

enum BackupRegister {
    BKP0 = 0,
    BKP1,
    BKP2,
    BKP3,
    BKP4,
    BKP5,
    BKP6,
    BKP7,
    BKP8,
    BKP9,
};

#define RTC_BKP_DR(reg)  MMIO16(BACKUP_REGS_BASE + 4 + (4 * (reg)))

extern void backup_write(enum BackupRegister reg, uint16_t value);
extern uint16_t backup_read(enum BackupRegister reg);

#endif