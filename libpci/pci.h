/*
 *	$Id: pci.h,v 1.12 2003/01/04 11:04:39 mj Exp $
 *
 *	The PCI Library
 *
 *	Copyright (c) 1997--2002 Martin Mares <mj@ucw.cz>
 *
 *	Can be freely distributed and used under the terms of the GNU GPL.
 */

#ifndef _PCI_LIB_H
#define _PCI_LIB_H

#include "header.h"

/*
 *	Types
 */

typedef unsigned char byte;
typedef unsigned char u8;
typedef unsigned short word;
typedef unsigned short u16;
typedef unsigned long u32;

typedef unsigned long pciaddr_t;

/*
 *	PCI Access Structure
 */

struct pci_methods;

enum pci_access_type {
  /* Known access methods, remember to update access.c as well */
  PCI_ACCESS_WINDOWS_HAL,		/* Windows Hardware Abstraction Layer */
  PCI_ACCESS_I386_TYPE1,		/* i386 ports, type 1 (params: none) */
  PCI_ACCESS_I386_TYPE2,		/* i386 ports, type 2 (params: none) */
  PCI_ACCESS_MAX
};

struct pci_access {
  /* Options you can change: */
  unsigned int method;			/* Access method */
  char *method_params[PCI_ACCESS_MAX];	/* Parameters for the methods */
  int buscentric;			/* Bus-centric view of the world */

  /* Functions you can override: */
  void (*error)(char *msg, ...);	/* Write error message and quit */
  void (*warning)(char *msg, ...);	/* Write a warning message */
  void (*debug)(char *msg, ...);	/* Write a debugging message */

  struct pci_dev *devices;		/* Devices found on this bus */

  /* Fields used internally: */
  struct pci_methods *methods;
};

/* Initialize PCI access */
struct pci_access *pci_alloc(void);
void pci_init(struct pci_access *);
void pci_cleanup(struct pci_access *);

/* Scanning of devices */
void pci_scan_bus(struct pci_access *acc);
struct pci_dev *pci_get_dev(struct pci_access *acc, word bus, byte dev, byte func); /* Raw access to specified device */
void pci_free_dev(struct pci_dev *);

/*
 *	Devices
 */

struct pci_dev {
  struct pci_dev *next;			/* Next device in the chain */
  word bus;				/* Higher byte can select host bridges */
  byte dev, func;			/* Device and function */

  /* These fields are set by pci_fill_info() */
  int known_fields;			/* Set of info fields already known */
  word vendor_id, device_id;		/* Identity of the device */
  int irq;				/* IRQ number */
  pciaddr_t base_addr[6];		/* Base addresses */
  pciaddr_t size[6];			/* Region sizes */
  pciaddr_t rom_base_addr;		/* Expansion ROM base address */
  pciaddr_t rom_size;			/* Expansion ROM size */

  /* Fields used internally: */
  struct pci_access *access;
  struct pci_methods *methods;
  int hdrtype;				/* Direct methods: header type */
  void *aux;				/* Auxillary data */
};

#define PCI_ADDR_IO_MASK (~(pciaddr_t) 0x3)
#define PCI_ADDR_MEM_MASK (~(pciaddr_t) 0xf)

byte pci_read_byte(struct pci_dev *, int pos); /* Access to configuration space */
word pci_read_word(struct pci_dev *, int pos);
u32  pci_read_long(struct pci_dev *, int pos);
int pci_read_block(struct pci_dev *, int pos, byte *buf, int len);
int pci_write_byte(struct pci_dev *, int pos, byte data);
int pci_write_word(struct pci_dev *, int pos, word data);
int pci_write_long(struct pci_dev *, int pos, u32 data);
int pci_write_block(struct pci_dev *, int pos, byte *buf, int len);

int pci_fill_info(struct pci_dev *, int flags); /* Fill in device information */

#define PCI_FILL_IDENT		1
#define PCI_FILL_IRQ		2
#define PCI_FILL_BASES		4
#define PCI_FILL_ROM_BASE	8
#define PCI_FILL_SIZES		16
#define PCI_FILL_RESCAN		0x10000


/*
 *	Filters
 */

struct pci_filter {
  int bus, slot, func;			/* -1 = ANY */
  int vendor, device;
};

void pci_filter_init(struct pci_access *, struct pci_filter *);
int pci_filter_match(struct pci_filter *, struct pci_dev *);

#define PCI_LOOKUP_VENDOR 1
#define PCI_LOOKUP_DEVICE 2
#define PCI_LOOKUP_CLASS 4
#define PCI_LOOKUP_SUBSYSTEM 8
#define PCI_LOOKUP_PROGIF 16
#define PCI_LOOKUP_NUMERIC 0x10000

#endif
