/*! \file
 
Module Name:	winflashrom.c

Abstract:   Main file of winflashrom device driver 

Author:	    Darmawan Salihun 

Environment:	Kernel mode

Revision History:
    
    - (June 2007) BIOS Probing device driver Modified to work with winflashrom
    - (July 29th) MMIO mapping limited to low 1MB and 4GB-20MB to 4GB physical address range
*/

#include "winflashrom.h"
#include <devioctl.h>
#include "../interfaces.h"

static const ULONG _1MB = 0x100000;
static const ULONG64 _4GB = 0x100000000 ;
static const ULONG _4GB_min_20MB = (0x100000000 - (20*0x100000)); 


NTSTATUS DriverEntry( IN PDRIVER_OBJECT  DriverObject, IN PUNICODE_STRING RegistryPath )
/*!

\brief 	Installable driver initialization entry point.
    	This entry point is called directly by the I/O system.


\param	DriverObject - pointer to the driver object

\param 	RegistryPath - pointer to a unicode string representing the path,
                   to driver-specific key in the registry.

\retval    STATUS_SUCCESS if successful,
\retval    STATUS_UNSUCCESSFUL otherwise

*/
{
    NTSTATUS            status = STATUS_SUCCESS;
    UNICODE_STRING      unicodeDeviceName;   
    UNICODE_STRING      unicodeDosDeviceName;  
    PDEVICE_OBJECT      deviceObject;
    PDEVICE_EXTENSION	pDevExt;
    ULONG		i;
    
    UNREFERENCED_PARAMETER (RegistryPath);

    WINFLASHROM_KDPRINT(("DriverEntry Enter \n"));

    DriverObject->DriverUnload = DispatchUnload;    

    DriverObject->MajorFunction[IRP_MJ_CREATE]= DispatchCreate;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = DispatchClose;
    DriverObject->MajorFunction[IRP_MJ_READ] = DispatchRead; 
    DriverObject->MajorFunction[IRP_MJ_WRITE] = DispatchWrite;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DispatchIoControl;

   
    (void) RtlInitUnicodeString(&unicodeDeviceName, WINFLASHROM_DEVICE_NAME_U);

    status = IoCreateDevice(
                DriverObject,
                sizeof(DEVICE_EXTENSION),
                &unicodeDeviceName,
                FILE_DEVICE_UNKNOWN,
                0,
                (BOOLEAN) FALSE,
                &deviceObject
                );

    if (!NT_SUCCESS(status))
    {
        return status;
    }

    WINFLASHROM_KDPRINT(("DeviceObject %p\n", deviceObject));

    //
    // Set the flag signifying that we will do direct I/O. This causes NT
    // to lock the user buffer into memory when it's accessed
    //
    deviceObject->Flags |= DO_DIRECT_IO;

    
    //
    // Allocate and initialize a Unicode String containing the Win32 name
    // for our device.
    //
    (void)RtlInitUnicodeString( &unicodeDosDeviceName, WINFLASHROM_DOS_DEVICE_NAME_U );


    status = IoCreateSymbolicLink(  (PUNICODE_STRING) &unicodeDosDeviceName,
				    (PUNICODE_STRING) &unicodeDeviceName );

    if (!NT_SUCCESS(status))
    {
        IoDeleteDevice(deviceObject);
        return status;
    }

    //
    // Initialize device extension
    //
    pDevExt = (PDEVICE_EXTENSION)deviceObject->DeviceExtension;
    for(i = 0; i < MAX_MAPPED_MMIO; i++) 
    {
	pDevExt->mapZone[i].sysAddrBase = NULL;
	pDevExt->mapZone[i].phyAddrStart = _4GB; // Note: _4GB is considered invalid phyAddrStart
	pDevExt->mapZone[i].size = 0;
	pDevExt->mapZone[i].usermodeAddrBase = NULL;
	pDevExt->mapZone[i].pMdl = NULL;
    }    
    
    WINFLASHROM_KDPRINT(("DriverEntry Exit = %x\n", status));

    return status;
}


NTSTATUS DispatchCreate( IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp )
/*!
\brief
   Process the create IRPs sent to this device.
   This routine does nothing but signalling 
   successful IRP handling.

\param 	DeviceObject - pointer to a device object.
\param	Irp - pointer to an I/O Request Packet.

\retval	NT Status code
*/
{
    NTSTATUS             status = STATUS_SUCCESS;

    WINFLASHROM_KDPRINT(("DispatchCreate Enter\n"));

    // 
    // The dispatch routine for IRP_MJ_CREATE is called when a 
    // file object associated with the device is created. 
    // This is typically because of a call to CreateFile() in 
    // a user-mode program or because another driver is 
    // layering itself over a this driver. A driver is 
    // required to supply a dispatch routine for IRP_MJ_CREATE.
    //
    WINFLASHROM_KDPRINT(("IRP_MJ_CREATE\n"));
    Irp->IoStatus.Information = 0;
    
    //
    // Save Status for return and complete Irp
    //
    Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    WINFLASHROM_KDPRINT((" DispatchCreate Exit = %x\n", status));

    return status;
}


NTSTATUS ReadPortByte(PIRP pIrp)
/*!
\brief
    Process the IRPs with IOCTL_READ_PORT_BYTE code. 
    This routine reads a byte from the designated port, 
    and returns the value to usermode application 
    through pointer to the locked-down usermode buffer 
    in the IRP.


\param	pIrp - pointer to an I/O Request Packet.

\retval	NT Status code
*/
{
    NTSTATUS status = STATUS_SUCCESS;
    IO_BYTE* pUsermodeMem =  (IO_BYTE*) MmGetSystemAddressForMdlSafe( pIrp->MdlAddress, NormalPagePriority );

    if( NULL != pUsermodeMem) {
        __asm
	{
	    pushad		    ;// save all register contents

	    mov ebx, pUsermodeMem ;// build user-mode memory pointer register	
	    mov dx,[ebx].port8	    ;// fetch input port addr
	    in  al,dx		    ;// read the byte from the device
	    mov [ebx].value8, al   ;// write probing result directly to user-mode memory

	    popad		    ;// restore all saved register value
	}
	
    } else {
	status = STATUS_INVALID_USER_BUFFER;
    }

    return status;
}


NTSTATUS ReadPortWord(PIRP pIrp)
/*!
\brief
    Process the IRPs with IOCTL_READ_PORT_WORD code. 
    This routine reads a word from the designated port, 
    and returns the value to usermode application 
    through pointer to the locked-down usermode buffer 
    in the IRP.

\param 	pIrp - pointer to an I/O Request Packet.

\retval	NT Status code
*/
{
    NTSTATUS status = STATUS_SUCCESS;
    IO_WORD* pUsermodeMem =  (IO_WORD*) MmGetSystemAddressForMdlSafe( pIrp->MdlAddress, NormalPagePriority );

    if( NULL != pUsermodeMem) {
        __asm
	{
	    pushad		    ;// save all register contents

	    mov ebx, pUsermodeMem	;// build user-mode memory pointer register	
	    mov dx, [ebx].port16	;// fetch input port addr
	    in  ax, dx			;// read the bytes from the device
	    mov [ebx].value16, ax	;// write probing result directly to user-mode memory

	    popad		    ;// restore all saved register value
	}
	
    } else {
	status = STATUS_INVALID_USER_BUFFER;
    }

    return status;
}


NTSTATUS ReadPortLong(PIRP pIrp)
/*!
\brief
    Process the IRPs with IOCTL_READ_PORT_LONG code. 
    This routine reads a DWORD from the designated port, 
    and returns the value to usermode application 
    through pointer to the locked-down usermode buffer 
    in the IRP.

\param	pIrp - pointer to an I/O Request Packet.

\retval	NT Status code
*/
{
    NTSTATUS status = STATUS_SUCCESS;
    IO_LONG* pUsermodeMem =  (IO_LONG*) MmGetSystemAddressForMdlSafe( pIrp->MdlAddress, NormalPagePriority );

    if( NULL != pUsermodeMem) {
        __asm
	{
	    pushad		    ;// save all register contents

	    mov ebx, pUsermodeMem	;// build user-mode memory pointer register	
	    mov dx, [ebx].port32	;// fetch input port addr
	    in  eax, dx			;// read the bytes from the device
	    mov [ebx].value32, eax	;// write probing result directly to user-mode memory

	    popad		    ;// restore all saved register value
	}
	
    } else {
	status = STATUS_INVALID_USER_BUFFER;
    }

    return status;
}


NTSTATUS WritePortByte(PIRP pIrp)
/*!
\brief
    Process the IRPs with IOCTL_WRITE_PORT_BYTE code. 
    This routine writes a byte to the designated port. 
    The value of the byte and the port address are obtained  
    through pointer to the locked-down buffer in the IRP.

\param 	pIrp - pointer to an I/O Request Packet.

\retval	NT Status code
--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    IO_BYTE* pUsermodeMem =  (IO_BYTE*) MmGetSystemAddressForMdlSafe( pIrp->MdlAddress, NormalPagePriority );

    if( NULL != pUsermodeMem) {
        __asm
	{
	    pushad		    ;// save all register contents

	    mov ebx, pUsermodeMem	;// build user-mode memory pointer register	
	    mov dx, [ebx].port8	;// fetch input port addr
    	    mov al, [ebx].value8	;// read the value to be written directly from user-mode memory
	    out dx, al			;// write the byte to the device

	    popad		    ;// restore all saved register value
	}
	
    } else {
	status = STATUS_INVALID_USER_BUFFER;
    }

    return status;
}


NTSTATUS WritePortWord(PIRP pIrp)
/*!
\brief
    Process the IRPs with IOCTL_WRITE_PORT_WORD code. 
    This routine writes a word to the designated port. 
    The value of the word and the port address are obtained  
    through pointer to the locked-down buffer in the IRP.

\param	pIrp - pointer to an I/O Request Packet.

\retval	NT Status code
*/
{
    NTSTATUS status = STATUS_SUCCESS;
    IO_WORD* pUsermodeMem =  (IO_WORD*) MmGetSystemAddressForMdlSafe( pIrp->MdlAddress, NormalPagePriority );

    if( NULL != pUsermodeMem) {
        __asm
	{
	    pushad		    ;// save all register contents

	    mov ebx, pUsermodeMem	;// build user-mode memory pointer register	
	    mov dx, [ebx].port16	;// fetch input port addr
    	    mov ax, [ebx].value16	;// read the value to be written directly from user-mode memory
	    out dx, ax			;// write the bytes to the device

	    popad		    ;// restore all saved register value
	}
	
    } else {
	status = STATUS_INVALID_USER_BUFFER;
    }

    return status;
}


NTSTATUS WritePortLong(PIRP pIrp)
/*!
\brief
    Process the IRPs with IOCTL_WRITE_PORT_LONG code. 
    This routine writes a dword to the designated port. 
    The value of the dword and the port address are obtained  
    through pointer to the locked-down buffer in the IRP.

\param	pIrp - pointer to an I/O Request Packet.

\retval	NT Status code
--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    IO_LONG* pUsermodeMem =  (IO_LONG*) MmGetSystemAddressForMdlSafe( pIrp->MdlAddress, NormalPagePriority );

    if( NULL != pUsermodeMem) {
        __asm
	{
	    pushad		    ;// save all register contents

	    mov ebx, pUsermodeMem	;// build user-mode memory pointer register	
	    mov dx, [ebx].port32	;// fetch input port addr
    	    mov eax, [ebx].value32	;// read the value to be written directly from user-mode memory
	    out dx, eax			;// write the bytes to the device

	    popad		    ;// restore all saved register value
	}
	
    } else {
	status = STATUS_INVALID_USER_BUFFER;
    }

    return status;
}


bool VerifyMmioAddressRange(ULONG64 phyAddrStart, ULONG size, PDEVICE_EXTENSION pDevExt)
/*!
\brief
   Verify whether the memory-mapped I/O address range 
   requested by the user mode application lies within 
   the "correct" address range that this driver will handle.

   This verification is needed to prevent user mode 
   application from mapping dangerous memory-mapped I/O 
   address range to user mode application

\param	phyAddrStart - starting physical address of the MMIO range to be mapped
\param	size - the size of the MMIO range to be mapped
\param	pDevExt - pointer to the DEVICE_EXTENSION of this driver

\retval false if the MMIO range contains "incorrect" address range
\retval true if the MMIO range doesn't contain "incorrect" address range
*/
{
  int i;	
  ULONG64 bottom, top ;

  WINFLASHROM_KDPRINT(("phyAddrStart = 0x%X \n", phyAddrStart));
  WINFLASHROM_KDPRINT(("size = 0x%X \n", size));
    
  /* First, check whether the MMIO range lies in the low 1MB range __or__ 
   * whether the MMIO range lies between the 4GB-20MB and 4GB limit 
   */

  if((phyAddrStart > _1MB ) && ((phyAddrStart + size) < _4GB_min_20MB)) {
  	return false;

  } else if (phyAddrStart > _4GB) {
	return false;
  }
  
  /* Now, check whether part or all of the requested physical address range 
   * has been mapped previously by the application that open access to this driver. 
   */ 
  
    for(i = 0; i < MAX_MAPPED_MMIO; i++)
    {
	if( pDevExt->mapZone[i].phyAddrStart != _4GB)
	{
      	  bottom = pDevExt->mapZone[i].phyAddrStart;
	  top = pDevExt->mapZone[i].phyAddrStart + pDevExt->mapZone[i].size;
      
	  WINFLASHROM_KDPRINT(("pDevExt->mapZone[%u].phyAddrStart = 0x%X \n", i, pDevExt->mapZone[i].phyAddrStart));
	  WINFLASHROM_KDPRINT(("pDevExt->mapZone[%u].size = 0x%X \n", i, pDevExt->mapZone[i].size));
          	  
	  if( (phyAddrStart < bottom ) && ((phyAddrStart + size) > bottom) ){ // enclosing or intersection in lower limit of mapZone[i]
	   	return false;
		
	  } else if ((phyAddrStart > bottom) && ((phyAddrStart + size) < top )) { // enclosed by mapZone[i]
	   	return false;
		
	  } else if ((phyAddrStart < top) && ((phyAddrStart + size) > top )) {  // enclosing or intersection in upper limit of mapZone[i]
		return false;			  
	  }    
   	}	
	
    }	    

    return true;
}



NTSTATUS MapMmio(PDEVICE_OBJECT pDO, PIRP pIrp)
/*!
\brief
    Process the IRPs with IOCTL_MAP_MMIO code. 
    This routine maps a physical address range 
    to the usermode application address space. 

    This function can only map the area 
    below the 4GB limit.

\param	pDO - pointer to the device object of this driver.
\param	pIrp - pointer to an I/O Request Packet.

\retval NT Status code
*/
{
    PDEVICE_EXTENSION pDevExt;
    PHYSICAL_ADDRESS phyAddr;
    MMIO_MAP* pUsermodeMem;
    ULONG   i, free_idx;
	    
    pDevExt = (PDEVICE_EXTENSION) pDO->DeviceExtension;

    //
    // Check for free mapZone in the device extension.
    // If none is free, return an error code.
    //
    for(i = 0; i < MAX_MAPPED_MMIO; i++)
    {
	if( pDevExt->mapZone[i].sysAddrBase == NULL )
	{
	    free_idx = i;
	    break;
	}	
    }	    
    
    if( i == MAX_MAPPED_MMIO )
    {
	return STATUS_UNSUCCESSFUL;
    }
    
    //
    // We have obtained a free mapZone, check whether the requested address range  
    // is valid and map the MMIO if it's valid
    //    
    pUsermodeMem =  (MMIO_MAP*) MmGetSystemAddressForMdlSafe( pIrp->MdlAddress, NormalPagePriority );
    if( NULL == pUsermodeMem) {
	return STATUS_INVALID_USER_BUFFER;
    } 
   
    if( false == VerifyMmioAddressRange(pUsermodeMem->phy_addr_start, pUsermodeMem->size, pDevExt)){
	return STATUS_UNSUCCESSFUL;
    }
	
    
    phyAddr.HighPart = 0;
    phyAddr.LowPart = pUsermodeMem->phy_addr_start;

    pDevExt->mapZone[free_idx].sysAddrBase = MmMapIoSpace( phyAddr, pUsermodeMem->size, MmNonCached);
    if(NULL == pDevExt->mapZone[free_idx].sysAddrBase) {
	return STATUS_BUFFER_TOO_SMALL;
    
    } else {
	pDevExt->mapZone[free_idx].phyAddrStart = pUsermodeMem->phy_addr_start;
    }
	    
    
    pDevExt->mapZone[free_idx].pMdl = IoAllocateMdl(pDevExt->mapZone[free_idx].sysAddrBase, 
						    pUsermodeMem->size, FALSE, FALSE, NULL);
    if(NULL == pDevExt->mapZone[free_idx].pMdl) 	    
    {
	MmUnmapIoSpace(pDevExt->mapZone[free_idx].sysAddrBase, pUsermodeMem->size);
	pDevExt->mapZone[free_idx].sysAddrBase = NULL;
	pDevExt->mapZone[free_idx].phyAddrStart =  _4GB;
	return STATUS_BUFFER_TOO_SMALL;
    }

    pDevExt->mapZone[free_idx].size = pUsermodeMem->size;

    //
    // Map the system virtual address to usermode virtual address
    // 
    MmBuildMdlForNonPagedPool(pDevExt->mapZone[free_idx].pMdl);
    pDevExt->mapZone[free_idx].usermodeAddrBase = MmMapLockedPagesSpecifyCache(	pDevExt->mapZone[free_idx].pMdl, 
										UserMode, MmNonCached, 
										NULL, FALSE,  
										NormalPagePriority);
    if(NULL ==  pDevExt->mapZone[free_idx].usermodeAddrBase)
    {
	IoFreeMdl(pDevExt->mapZone[free_idx].pMdl);
	MmUnmapIoSpace(pDevExt->mapZone[free_idx].sysAddrBase, pDevExt->mapZone[free_idx].size);
	pDevExt->mapZone[free_idx].sysAddrBase = NULL;
	pDevExt->mapZone[free_idx].phyAddrStart =  _4GB;
	pDevExt->mapZone[free_idx].size = 0;
	return STATUS_BUFFER_TOO_SMALL;
    } 

    // copy the resulting usermode virtual address to IRP "buffer"
    pUsermodeMem->usermode_virt_addr = pDevExt->mapZone[free_idx].usermodeAddrBase;
    
    return STATUS_SUCCESS;
}


NTSTATUS CleanupMmioMapping(PDEVICE_EXTENSION pDevExt, ULONG i)
/*!
\brief
    This routine cleanup the mapping of a MMIO range 
    and resources it consumes.
    

\param	pDevExt - pointer to the device extension of the driver
\param	i - index of the mapZone to cleanup

\retval	NT Status code
*/
{
    if( NULL != pDevExt->mapZone[i].usermodeAddrBase )
    {
	MmUnmapLockedPages( pDevExt->mapZone[i].usermodeAddrBase, 
			    pDevExt->mapZone[i].pMdl); 
	pDevExt->mapZone[i].usermodeAddrBase = NULL;
    }
	    
    if( NULL != pDevExt->mapZone[i].pMdl )
    {
	IoFreeMdl(pDevExt->mapZone[i].pMdl); 
	pDevExt->mapZone[i].pMdl = NULL;
    }
	
    if( NULL != pDevExt->mapZone[i].sysAddrBase )
    {
        MmUnmapIoSpace(	pDevExt->mapZone[i].sysAddrBase, 
			pDevExt->mapZone[i].size); 
        pDevExt->mapZone[i].sysAddrBase = NULL;
	pDevExt->mapZone[i].phyAddrStart = _4GB;
	pDevExt->mapZone[i].size = 0;
    } 

    return STATUS_SUCCESS; 
}

	
NTSTATUS UnmapMmio(PDEVICE_OBJECT pDO, PIRP pIrp)
/*!
\brief
    Process the IRPs with IOCTL_UNMAP_MMIO code. 
    This routine unmaps a previously mapped physical 
    address range.

    This function can only unmap the area 
    below the 4-GB limit.


\param	pDO - pointer to the device object of this driver.
\param	pIrp - pointer to an I/O Request Packet.

\retval	NT Status code
*/
{
    PDEVICE_EXTENSION pDevExt;
    MMIO_MAP* pMmioMap;
    ULONG i;
    
    // 
    // Unmap the requested zone from the system address space 
    // and update the device extension data
    // 
    pDevExt = (PDEVICE_EXTENSION) pDO->DeviceExtension;
    pMmioMap = (MMIO_MAP*) MmGetSystemAddressForMdlSafe( pIrp->MdlAddress, NormalPagePriority );

    for(i = 0 ; i < MAX_MAPPED_MMIO; i++)
    {
	if(pDevExt->mapZone[i].usermodeAddrBase == pMmioMap->usermode_virt_addr)
	{
	    CleanupMmioMapping(pDevExt, i);
	    break;
	}
    }
    
    return STATUS_SUCCESS;
}


NTSTATUS DispatchIoControl( IN PDEVICE_OBJECT pDO, IN PIRP pIrp )
/*!
\brief
    Io control code dispatch routine
           
\param	pDO - pointer to a device object.
\param	pIrp  - pointer to current Irp
  
\retval	NT status code.
*/
{
    NTSTATUS status = STATUS_SUCCESS;
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(pIrp);
	
    switch(irpStack->Parameters.DeviceIoControl.IoControlCode)
    {
	case IOCTL_READ_PORT_BYTE:
	    {
		if(irpStack->Parameters.DeviceIoControl.InputBufferLength >= sizeof(IO_BYTE)) {	
		    status = ReadPortByte(pIrp);
				
		} else {
		    status = STATUS_BUFFER_TOO_SMALL;
		}
	    }break;

	case IOCTL_READ_PORT_WORD:
	    {
		if(irpStack->Parameters.DeviceIoControl.InputBufferLength >= sizeof(IO_WORD)) {	
		    status = ReadPortWord(pIrp);
				
		} else {
		    status = STATUS_BUFFER_TOO_SMALL;
		}
	    }break;

	case IOCTL_READ_PORT_LONG:
	    {
		if(irpStack->Parameters.DeviceIoControl.InputBufferLength >= sizeof(IO_LONG)) {	
		    status = ReadPortLong(pIrp);
				
		} else {
		    status = STATUS_BUFFER_TOO_SMALL;
		}
	    }break;

	case IOCTL_WRITE_PORT_BYTE:
	    {
		if(irpStack->Parameters.DeviceIoControl.InputBufferLength >= sizeof(IO_BYTE)) {	
		    status = WritePortByte(pIrp);
				
		} else {
		    status = STATUS_BUFFER_TOO_SMALL;
		}
	    }break;

	case IOCTL_WRITE_PORT_WORD:
	    {
		if(irpStack->Parameters.DeviceIoControl.InputBufferLength >= sizeof(IO_WORD)) {	
		    status = WritePortWord(pIrp);
				
		} else {
		    status = STATUS_BUFFER_TOO_SMALL;
		}
	    }break;

	case IOCTL_WRITE_PORT_LONG:
	    {
		if(irpStack->Parameters.DeviceIoControl.InputBufferLength >= sizeof(IO_LONG)) {	
		    status = WritePortLong(pIrp);
				
		} else {
		    status = STATUS_BUFFER_TOO_SMALL;
		}
	    }break;

	case IOCTL_MAP_MMIO:
	    {
		if(irpStack->Parameters.DeviceIoControl.InputBufferLength >= sizeof(MMIO_MAP)) {	
		    status = MapMmio(pDO, pIrp);
				
		} else {
		    status = STATUS_BUFFER_TOO_SMALL;
		}
	    }break;
	    
	case IOCTL_UNMAP_MMIO:
	    {
		if(irpStack->Parameters.DeviceIoControl.InputBufferLength >= sizeof(MMIO_MAP)) {	
		    status = UnmapMmio(pDO, pIrp);
				
		} else {
		    status = STATUS_BUFFER_TOO_SMALL;
		}
	    }break;
	    
	default:	    
	    {
		status = STATUS_INVALID_DEVICE_REQUEST;
	    }break;
    }
    
    //
    // complete the I/O request and return appropriate values
    //
    pIrp->IoStatus.Status = status;
    
    // Set number of bytes to copy back to user-mode
    if(status == STATUS_SUCCESS)
    {
	pIrp->IoStatus.Information = irpStack->Parameters.DeviceIoControl.OutputBufferLength;
    }
    else
    {
	pIrp->IoStatus.Information = 0;
    }
    IoCompleteRequest( pIrp, IO_NO_INCREMENT );

    return status;
}


NTSTATUS DispatchRead( IN PDEVICE_OBJECT pDO, IN PIRP pIrp )
/*!
\brief
    Read dispatch routine

    This function does nothing. It's merely a place holder	 
    to satisfy the need of the user mode code to open the driver 
    with a GENERIC_READ parameter.
 
\param	pDO - pointer to a device object.
\param	pIrp  - pointer to current Irp
  
\retval	NT status code.
*/
{
    // Just complete the I/O request right away
    pIrp->IoStatus.Status = STATUS_SUCCESS;
    pIrp->IoStatus.Information = 0;
    IoCompleteRequest( pIrp, IO_NO_INCREMENT );

    return STATUS_SUCCESS;
}


NTSTATUS DispatchWrite( IN PDEVICE_OBJECT pDO, IN PIRP pIrp )
/*!
\brief
    Write dispatch routine

    This function does nothing. It's merely a place holder	 
    to satisfy the need of the user mode code to open the driver 
    with a GENERIC_WRITE parameter.
    
\param	pDO - pointer to a device object.
\param	pIrp  - pointer to current Irp
  
\retval	NT status code.
*/
{
    // Just complete the I/O request right away
    pIrp->IoStatus.Status = STATUS_SUCCESS;
    pIrp->IoStatus.Information = 0;
    IoCompleteRequest( pIrp, IO_NO_INCREMENT );

    return STATUS_SUCCESS;
}


NTSTATUS DispatchClose( IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp )
/*!
\brief
   Process the close IRPs sent to this device.

   This function clean-up the mapped MMIO ranges that 
   haven't been cleaned-up by a "buggy" usermode application.

\param	DeviceObject - pointer to a device object.
\param	Irp - pointer to an I/O Request Packet.

\retval NT Status code
*/
{
    PDEVICE_EXTENSION pDevExt;
    ULONG	i;
    NTSTATUS	status = STATUS_SUCCESS;
    
    WINFLASHROM_KDPRINT(("DispatchClose Enter\n"));

    pDevExt = DeviceObject->DeviceExtension ;
    
    //
    // Clean-up the mapped MMIO space in case the usermode
    // application forget to call UnmapMmio for some MMIO zone.
    // This is to guard against some buggy usermode application.
    //
    for(i = 0; i < MAX_MAPPED_MMIO; i++)
    {
	if(pDevExt->mapZone[i].sysAddrBase != NULL)
	{
	    CleanupMmioMapping(pDevExt, i);
	}
    }
    
    //
    // The IRP_MJ_CLOSE dispatch routine is called when a file object
    // opened on the driver is being removed from the system; that is,
    // all file object handles have been closed and the reference count
    // of the file object is down to 0. 
    //
    WINFLASHROM_KDPRINT(("IRP_MJ_CLOSE\n"));
    Irp->IoStatus.Information = 0;

    //
    // Save Status for return and complete Irp
    //
    Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    WINFLASHROM_KDPRINT((" DispatchClose Exit = %x\n", status));

    return status;
}


VOID DispatchUnload( IN PDRIVER_OBJECT DriverObject )
/*!
\brief
    Free all the allocated resources, etc.

\param	DriverObject - pointer to a driver object.

\retval	VOID
*/
{
    PDEVICE_OBJECT  deviceObject = DriverObject->DeviceObject;
    UNICODE_STRING  uniWin32NameString;

    WINFLASHROM_KDPRINT(("DispatchUnload Enter\n"));

    //
    // Create counted string version of our Win32 device name.
    //

    RtlInitUnicodeString( &uniWin32NameString, WINFLASHROM_DOS_DEVICE_NAME_U );

    IoDeleteSymbolicLink( &uniWin32NameString );

    ASSERT(!deviceObject->AttachedDevice);
    
    IoDeleteDevice( deviceObject );
 
    WINFLASHROM_KDPRINT(("DispatchUnload Exit\n"));
    return;
}


