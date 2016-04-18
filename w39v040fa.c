/*
 * w39v040fa.c: driver for Winbond 39V040FA flash models
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program; if not, write to the Free Software
 *	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *
 * Reference:
 *	W39V040FA data sheet
 *
 */

#include <stdio.h>
#include "flash.h"
#include "jedec.h"
#include "w39v040fa.h"
#include "direct_io.h"

enum {
	BLOCKING_REGS_PHY_RANGE = 0x80000,
	BLOCKING_REGS_PHY_BASE = 0xFFB80000,
};


static volatile char * unprotect_39v040fa(void)
{
	unsigned char i, byte_val;
	volatile char * block_regs_base;

	block_regs_base = (volatile char*) map_physical_addr_range( BLOCKING_REGS_PHY_BASE, BLOCKING_REGS_PHY_RANGE);
	if (block_regs_base == NULL) {
		perror("Error: Unable to map Winbond w39v040fa blocking registers!\n");
		return NULL;
	}

	// 
	// Unprotect the BIOS chip address range
	//
	for( i = 0; i < 8 ; i++ )
	{
		byte_val =  *(block_regs_base + 2 + i*0x10000);
		myusec_delay(10);
		byte_val &= 0xF8; // Enable full access to the chip
		*(block_regs_base + 2 + i*0x10000) = byte_val;
		myusec_delay(10);
	}

	return block_regs_base;
}


static void protect_39v040fa(volatile char * reg_base)
{
	//
	// Protect the BIOS chip address range
	//
	unsigned char i, byte_val;
	volatile char * block_regs_base = reg_base;

	for( i = 0; i < 8 ; i++ )
	{
		byte_val = *(block_regs_base + 2 + i*0x10000);
		myusec_delay(10);
		byte_val |= 1; // Prohibited to write in the block where set
		*(block_regs_base + 2 + i*0x10000) = byte_val;
		myusec_delay(10);
	}

	unmap_physical_addr_range((void*) reg_base, BLOCKING_REGS_PHY_RANGE);
}


int write_39v040fa(struct flashchip *flash, uint8_t *buf)
{
	int i;
	int total_size = flash->total_size * 1024;
	int page_size = flash->page_size;
	volatile uint8_t *bios = flash->virtual_memory;
	volatile char * reg_base;
	
	reg_base = unprotect_39v040fa();
	erase_chip_jedec(flash);

	printf("Programming Page: ");
	for (i = 0; i < total_size / page_size; i++) {
		/* write to the sector */
		printf("%04d at address: 0x%08x", i, i * page_size);
		write_sector_jedec(bios, buf + i * page_size,
				   bios + i * page_size, page_size);
		printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");
		fflush(stdout);
	}
	printf("\n");

	if(NULL != reg_base)
	{
	    protect_39v040fa(reg_base);
	}
	
	return (0);
}
