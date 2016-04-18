#ifndef __DIRECT_IO_H__
#define __DIRECT_IO_H__

int init_driver(); // returns 0 on error, 1 on success
void cleanup_driver(); // must be called after done using the driver

void* map_physical_addr_range( unsigned long phy_addr_start, 
						 unsigned long size ); // returns NULL on error, valid pointer on success
int unmap_physical_addr_range( void* virt_addr_start, 
		        unsigned long size ); // must be called after done with the mapped physical memory

unsigned long hal_pci_read_offset( unsigned long  bus, unsigned long  device,
		                   unsigned long  function, unsigned long  offset, 
				   unsigned char length);

void hal_pci_write_offset( unsigned long  bus, unsigned long  device, 
			   unsigned long  function, unsigned long  offset, 
			   unsigned long value, unsigned char length );

void outb(unsigned char value, unsigned short port);
void outw(unsigned short value, unsigned short port);
void outl(unsigned long value, unsigned short port);

unsigned char inb(unsigned short port);
unsigned short inw(unsigned short port);
unsigned long inl(unsigned short port);

#endif //__DIRECT_IO_H__
