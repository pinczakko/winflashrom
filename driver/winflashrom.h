#ifndef __WINFLASHROM_H__
#define __WINFLASHROM_H__

#include <ntddk.h>
#include "../interfaces.h"

//  Debugging macros

#if DBG
#define WINFLASHROM_KDPRINT(_x_) \
                DbgPrint("WINFLASHROM.SYS: ");\
                DbgPrint _x_;
#else

#define WINFLASHROM_KDPRINT(_x_)

#endif



#define WINFLASHROM_DEVICE_NAME_U     L"\\Device\\winflashrom"
#define WINFLASHROM_DOS_DEVICE_NAME_U L"\\DosDevices\\winflashrom"


typedef struct _MMIO_RING_0_MAP{
    PVOID  sysAddrBase;	    	// start of system virtual address of the mapped physical address range 
    ULONG64 phyAddrStart;    	// start of physical address of the mapped MMIO
    ULONG  size;		// size of the mapped physical address range 
    PVOID  usermodeAddrBase; 	// pointer to the usermode virtual address where this range is mapped
    PMDL pMdl;		    	// Memory Descriptor List for the memory mapped I/O range to be mapped
}MMIO_RING_0_MAP, *PMMIO_RING_0_MAP;

typedef struct _DEVICE_EXTENSION{
    MMIO_RING_0_MAP mapZone[MAX_MAPPED_MMIO];   
}DEVICE_EXTENSION, *PDEVICE_EXTENSION;

typedef enum {false = 0, true = 1} bool;

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT  DriverObject,
    IN PUNICODE_STRING registryPath
);

NTSTATUS
DispatchCreate(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
DispatchClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
DispatchUnload(
    IN PDRIVER_OBJECT DriverObject
    );

NTSTATUS
DispatchRead(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);


NTSTATUS
DispatchWrite(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
DispatchIoControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

#endif //__WINFLASHROM_H__


