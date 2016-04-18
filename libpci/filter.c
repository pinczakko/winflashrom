/*
 *	$Id: filter.c,v 1.3 2002/03/30 15:39:25 mj Exp $
 *
 *	Linux PCI Library -- Device Filtering
 *
 *	Copyright (c) 1998--2002 Martin Mares <mj@ucw.cz>
 *
 *	Can be freely distributed and used under the terms of the GNU GPL.
 */

#include <stdlib.h>
#include <string.h>

#include "internal.h"

void
pci_filter_init(struct pci_access * a, struct pci_filter *f)
{
  f->bus = f->slot = f->func = -1;
  f->vendor = f->device = -1;
}


int
pci_filter_match(struct pci_filter *f, struct pci_dev *d)
{
  if ((f->bus >= 0 && f->bus != d->bus) ||
      (f->slot >= 0 && f->slot != d->dev) ||
      (f->func >= 0 && f->func != d->func))
    return 0;
  if (f->device >= 0 || f->vendor >= 0)
    {
      pci_fill_info(d, PCI_FILL_IDENT);
      if ((f->device >= 0 && f->device != d->device_id) ||
	  (f->vendor >= 0 && f->vendor != d->vendor_id))
	return 0;
    }
  return 1;
}
