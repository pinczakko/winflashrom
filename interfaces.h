/*
 *  This is the interface file that connects the user mode application and the kernel mode driver
 *
 *  NOTE:
 *  -----
 *  - You __have__ to put #include <winioctl.h> prior to including this file in your user mode application
 *  - You probably need to put #include <devioctl.h> prior to including this file in your kernel mode driver 
 *  These includes are needed for the CTL_CODE macro to work.
 */

#ifndef __INTERFACES_H__
#define __INTERFACES_H__

#define IOCTL_READ_PORT_BYTE	CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0801, METHOD_IN_DIRECT, FILE_READ_DATA | FILE_WRITE_DATA)
#define IOCTL_READ_PORT_WORD	CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0802, METHOD_IN_DIRECT, FILE_READ_DATA | FILE_WRITE_DATA)
#define IOCTL_READ_PORT_LONG	CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0803, METHOD_IN_DIRECT, FILE_READ_DATA | FILE_WRITE_DATA)

#define IOCTL_WRITE_PORT_BYTE	CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0804, METHOD_OUT_DIRECT, FILE_READ_DATA | FILE_WRITE_DATA)
#define IOCTL_WRITE_PORT_WORD	CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0805, METHOD_OUT_DIRECT, FILE_READ_DATA | FILE_WRITE_DATA)
#define IOCTL_WRITE_PORT_LONG	CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0806, METHOD_OUT_DIRECT, FILE_READ_DATA | FILE_WRITE_DATA)

#define IOCTL_MAP_MMIO		CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0809, METHOD_IN_DIRECT, FILE_READ_DATA | FILE_WRITE_DATA)
#define IOCTL_UNMAP_MMIO	CTL_CODE(FILE_DEVICE_UNKNOWN, 0x080A, METHOD_OUT_DIRECT, FILE_READ_DATA | FILE_WRITE_DATA)


#ifdef __GNUC__ // are we using GCC to compile this code?
#define PACK_STRUC __attribute__((packed))
#else
#define PACK_STRUC 
#endif 


enum {
    MAX_MAPPED_MMIO = 256 // maximum number of mapped memory-mapped I/O zones
};

#ifdef _MSC_VER // are we using MSVC to compile this code?
#pragma pack (push, 1)
#endif

struct _IO_BYTE {
    unsigned short port8;
    unsigned char value8;
}PACK_STRUC; 
typedef struct _IO_BYTE IO_BYTE;

struct _IO_WORD {
    unsigned short port16; 
    unsigned short value16;
}PACK_STRUC;
typedef struct _IO_WORD IO_WORD;

struct _IO_LONG {
    unsigned short port32;
    unsigned long value32;
}PACK_STRUC;
typedef struct _IO_LONG IO_LONG; 

struct _MMIO_MAP {
    unsigned long phy_addr_start; // start address in physical address space to be mapped
    unsigned long size; // size of the physical address space to map
    void * usermode_virt_addr; // starting virtual address of the MMIO as seen from user mode
}PACK_STRUC;
typedef struct _MMIO_MAP MMIO_MAP;

struct _PCI_ADDRESS {
	unsigned long bus;
	unsigned long device;
	unsigned long function;
	unsigned long value;
	unsigned char offset;
	unsigned char length; // length of the value to be written/read in bytes
}PACK_STRUC;
typedef struct _PCI_ADDRESS PCI_ADDRESS;

#ifdef _MSC_VER // are we using MSVC to compile this code?
#pragma pack (pop)
#endif


#endif //__INTERFACES_H__
