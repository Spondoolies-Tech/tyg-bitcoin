/****************************************************************************/
/*																			*/
/*	Module:			jamport.h												*/
/*																			*/
/*					Copyright (C) Altera Corporation 2000					*/
/*																			*/
/*	Description:	Defines porting macros									*/
/*																			*/
/****************************************************************************/

#ifndef INC_JAMPORT_H
#define INC_JAMPORT_H

/*
*	PORT defines the target platform -- should be DOS, WINDOWS, or UNIX
*
*	PORT = DOS     means a 16-bit DOS console-mode application
*
*	PORT = WINDOWS means a 32-bit WIN32 console-mode application for
*	               Windows 95 or Windows NT.  On NT this will use the
*	               DeviceIoControl() API to access the Parallel Port.
*
*	PORT = UNIX    means any UNIX system.  BitBlaster access is support via
*	               the standard ANSI system calls open(), read(), write().
*	               The ByteBlaster is not supported.
*
*	PORT = EMBEDDED means all DOS, WINDOWS, and UNIX code is excluded. Remaining
*			code supports 16 and 32-bit compilers. Additional porting
*			steps may be necessary. See readme file for more details.
*/

#include <stdint.h>

#define DOS      2
#define WINDOWS  3
#define UNIX     4
#define EMBEDDED 5

/* change this line to build a different port */
#define PORT EMBEDDED

/* JTAG Configuration Signals */
#define SIG_TCK  0 /* TCK */
#define SIG_TMS  1 /* TMS */
#define SIG_TDI  2 /* TDI */
#define SIG_TDO  3 /* TDO */


typedef struct
{
	uint32_t	dummy0[0x138 / sizeof(uint32_t)];
	uint32_t	datain;
	uint32_t	dataout;
	uint32_t	dummy1[(0x190 - 0x138 - (2 * sizeof(uint32_t)))/ sizeof(uint32_t)];
	uint32_t	cleardataout;
	uint32_t	setdataout;
} gpio_bank_t;

extern volatile	gpio_bank_t	*gpio2_bank;
extern volatile	gpio_bank_t	*gpio3_bank;

static const uint32_t	signals[] =
{
	[SIG_TCK]	= 1 << 25,	/* gpio2, out */
	[SIG_TMS]	= 1 << 24,	/* gpio2, out */
	[SIG_TDI]	= 1 << 22,	/* gpio2, out */

	[SIG_TDO]	= 1 << 21,	/* gpio3, in */
};

#endif /* INC_JAMPORT_H */
