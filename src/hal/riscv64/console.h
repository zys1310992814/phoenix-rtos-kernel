/*
 * Phoenix-RTOS
 *
 * Operating system kernel
 *
 * HAL console (RISCV64)
 *
 * Copyright 2018 Phoenix Systems
 * Author: Pawel Pisarczyk
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _HAL_CONSOLE_H_
#define _HAL_CONSOLE_H_

#include "cpu.h"


/* Console attributes */
#define ATTR_NORMAL  0x03
#define ATTR_BOLD    0x0f
#define ATTR_USER    0x07


#ifndef __ASSEMBLY__

extern void hal_consolePrint(int attr, const char *s);

extern void _hal_consoleInit(void);

#endif

#endif
