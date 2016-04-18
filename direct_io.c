#include <windows.h>
#include <winioctl.h>
#include <stdio.h>

#include "interfaces.h" // Must be included after winioctl.h
#include "direct_io.h"
#include "error_msg.h"

#define DRIVER_NAME       "winflashrom"

//
// verbosity of messages
//
#define VEBOSE_DEBUG_MESSAGE
#undef VEBOSE_DEBUG_MESSAGE

//
// Globals
//
static HANDLE h_device = INVALID_HANDLE_VALUE;
static UCHAR driver_path[MAX_PATH];

//
// Function implementation
//

static BOOLEAN setup_driver_name( PUCHAR driver_path )
{
    HANDLE file_handle;
    unsigned long err_no;
    unsigned long driver_path_len = 0;

    //
    // Get the current directory.
    //
    driver_path_len = GetCurrentDirectory(MAX_PATH, driver_path );

    if (!driver_path_len) {
	err_no = GetLastError();	    
        printf("GetCurrentDirectory failed!  Error = %d \n",(int) err_no);
				display_error_message(err_no);

        return FALSE;
    }

    //
    // Setup path name to driver file.
    //

    strcat(driver_path, "\\");
    strcat(driver_path, DRIVER_NAME);
    strcat(driver_path, ".sys");

    //
    // Insure driver file is in the specified directory.
    //

    if ((file_handle = CreateFile(driver_path,
                                 GENERIC_READ | GENERIC_WRITE,
                                 0,
                                 NULL,
                                 OPEN_EXISTING,
                                 FILE_ATTRIBUTE_NORMAL,
                                 NULL
                                 )) == INVALID_HANDLE_VALUE) {


        printf("Driver: BIOS_PROBE.SYS is not in the system directory. \n");

        //
        // Indicate failure.
        //

        return FALSE;
    }

    //
    // Close open file handle.
    //

    if (file_handle) {

        CloseHandle(file_handle);
    }

    //
    // Indicate success.
    //

    return TRUE;


}   // setup_driver_name


static BOOL extract_driver(char *sz_driver, char *sz_out)
/*++
Routine Description:
	find, load and extract the driver from resource

Arguments: 
	sz_driver = resource name or number
	sz_out    = output filename
    
Return Value:
	FALSE = function fails
	TRUE = function succeds
--*/

{
	HRSRC h_res;
	HGLOBAL h_sys;
	LPVOID p_sys;
	HANDLE h_file;
	unsigned long dw_size, dw_wri;

	// find and load resources, return false if failed
	if(!(h_res = FindResource(NULL, sz_driver, TEXT("DRIVER"))))
		return FALSE;

	if(!(h_sys = LoadResource(0, h_res)))
		return FALSE;

	dw_size = SizeofResource(0, h_res);
	p_sys = LockResource(h_sys);

	// create the driver file to the choosen path
	if((h_file = CreateFile(sz_out, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0)) == INVALID_HANDLE_VALUE)
		return FALSE;

	// write the driver and and close the file handle
	if(!WriteFile(h_file, p_sys, dw_size, &dw_wri, 0))
	{
		CloseHandle(h_file);
	 	return FALSE;
	}
	
	CloseHandle(h_file);
	return TRUE;
}


static BOOL activate_driver(char *sz_serv, char *sz_path, BOOL activate)
/*++
Routine Description:
	register, run, stop and unregister driver

Arguments: 
	sz_serv   = driver service name
	sz_path   = path to driver
	activate = register/unregisterNone
    
Return Value:
	0 = function fails
	1 = function succeds
--*/
{
	BOOL b_stat;
	SC_HANDLE h_sc, h_sr;
	SERVICE_STATUS srv_stat;

	b_stat = FALSE;
	
	h_sc = OpenSCManager(0, 0, SC_MANAGER_ALL_ACCESS);
	if(!h_sc) 
	{
	    printf("Error: failed to open service control manager\n");
	    return FALSE;
	}

	if(activate)
	{
	    // start debug		
 	    printf("creating new service... \n");
	    // end debug
	    
	    h_sr = CreateService(h_sc, sz_serv, sz_serv, SERVICE_ALL_ACCESS, SERVICE_KERNEL_DRIVER, 
				SERVICE_DEMAND_START, SERVICE_ERROR_NORMAL, sz_path, 
				NULL, NULL, NULL, NULL, NULL);
		
	    if(!h_sr) 
	    {
	    	// start debug		
 	    	printf("invalid service handle... \n");
	    	// end debug
		    
		if(GetLastError() == ERROR_SERVICE_EXISTS)
		{
		    h_sr = OpenService(h_sc, sz_serv, SERVICE_ALL_ACCESS);
		    
		    if(!h_sr)
		    {
			printf("Error: failed to open access to the service (service already exists)\n");
			    
			CloseServiceHandle(h_sc);
			return FALSE;
		    }

		    printf("Error: service already exist!\n");
		}   
		else
		{
		    // start debug
		    printf("CreateService failed. But, not because of service exists :(\n");
		    // end debug
		    
		    CloseServiceHandle(h_sc);
		    return FALSE;
		}
	    }
	    else
	    {
		if(StartService(h_sr, 0, NULL) == 0) 
		{
		    if(ERROR_SERVICE_ALREADY_RUNNING != GetLastError())
		    {    
                        CloseServiceHandle(h_sc);
                        CloseServiceHandle(h_sr);
                        return FALSE;
		    }
		} 
		else 
		{
		    b_stat = TRUE;
		}
	    }
	}
	else
	{
		if(!(h_sr = OpenService(h_sc, sz_serv, SERVICE_ALL_ACCESS)))
			b_stat = FALSE;
		else
			b_stat = (ControlService(h_sr, SERVICE_CONTROL_STOP, &srv_stat) == 0) ? FALSE : TRUE;

		// unregister the driver
		b_stat = (DeleteService(h_sr) == 0) ? FALSE : TRUE;
	}

	// close the service handle
	if(h_sr) CloseServiceHandle(h_sr);
	if(h_sc) CloseServiceHandle(h_sc);
	return b_stat;
}


int init_driver()
/*++
Routine Description:
	Initialize the driver interface

Arguments: None
    
Return Value:
    0 if error
    1 if succeeded
--*/
{
	unsigned long err_no;

	// 
    // Extract the driver binary from the resource in the executable
    // 
    if (extract_driver(TEXT("WINFLASHROMDRV"), "winflashrom.sys") == TRUE) {
		printf("The driver has been extracted\n");
		
    } else {
		display_error_message(GetLastError());
		printf("Exiting..\n");
		return 0;
    }
    
    
    //
    // Setup full path to driver name.
    //
    if (!setup_driver_name(driver_path)) {
		printf("Error: failed to setup driver name \n");
        return 0;
    }


    //
    // Try to activate the driver 
    //
    if(activate_driver(DRIVER_NAME, driver_path, TRUE) == TRUE) {
		printf("The driver is registered and activated\n"); 
    } else {
		printf("Error: unable to register and activate the driver\n");
		DeleteFile(driver_path);
		return 0;		
    }

    //
    // Try to open the newly installed driver.
    //
        
    h_device = CreateFile( "\\\\.\\winflashrom",
                GENERIC_READ | GENERIC_WRITE,
                0,
                NULL,
                OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL,
                NULL);

    if ( h_device == INVALID_HANDLE_VALUE ){
        err_no = GetLastError();	
		printf ( "Error: CreateFile Failed : %d\n", (int) err_no );
		display_error_message(err_no);

		// cleanup resurces created and used up to now
		activate_driver(DRIVER_NAME, driver_path, FALSE);
		DeleteFile(driver_path);
	
        return 0;
    }

	return 1;
}


void cleanup_driver()
{
    //
    // Close the access to the exported device interface in user-mode
    //
	if(h_device != INVALID_HANDLE_VALUE) {
		CloseHandle(h_device); 
	}

    //
    // Unload the driver.  Ignore any errors.
    //
    if(activate_driver(DRIVER_NAME, driver_path, FALSE) == TRUE) {
		printf("The driver stopped and unloaded\n"); 
    } else {
		printf("Error: failed to stop and unload the driver\n");
    }

    //
    // Delete the driver file
    // 
    DeleteFile(driver_path);
}



unsigned char inb(unsigned short port)
{
    IO_BYTE ioBuf;
    unsigned long bytes_returned;
    
    ioBuf.port8 = port; // port address to read from
    

    if( INVALID_HANDLE_VALUE == h_device) {
	printf("(inl) Error: the driver handle is invalid!\n");
	return -1;
    }
    
    if( FALSE == DeviceIoControl( h_device, 
				 IOCTL_READ_PORT_BYTE,
				 &ioBuf,
				 sizeof(ioBuf),
				 &ioBuf,
				 sizeof(ioBuf),
				 &bytes_returned,
				 NULL))
    {
	display_error_message(GetLastError());
	return -1;
    }

#ifdef VEBOSE_DEBUG_MESSAGE
    printf("number of bytes returned from kernel (IOCTL_READ_PORT_BYTE) : %d\n", 
	    bytes_returned);
#endif //VEBOSE_DEBUG_MESSAGE

    return ioBuf.value8;
}


void outb(unsigned char value, unsigned short port)
{
    IO_BYTE ioBuf;
    unsigned long bytes_returned;
    
    ioBuf.port8 = port; // port address to write to
    ioBuf.value8 = value; 

    if( INVALID_HANDLE_VALUE == h_device) {
	printf("(outw) Error: the driver handle is invalid!\n");
	return;
    }
    
    if( FALSE == DeviceIoControl( h_device, 
				 IOCTL_WRITE_PORT_BYTE,
				 &ioBuf,
				 sizeof(ioBuf),
				 &ioBuf,
				 sizeof(ioBuf),
				 &bytes_returned,
				 NULL))
    {
	display_error_message(GetLastError());
    }
    
 #ifdef VEBOSE_DEBUG_MESSAGE
    printf("number of bytes returned from kernel (IOCTL_WRITE_PORT_BYTE) : %d\n", 
	    bytes_returned);
 #endif //VEBOSE_DEBUG_MESSAGE
}


unsigned short inw(unsigned short port)
{
    IO_WORD ioBuf;
    unsigned long bytes_returned;
    
    ioBuf.port16 = port; // port address to read from
    

    if( INVALID_HANDLE_VALUE == h_device) {
	printf("(inl) Error: the driver handle is invalid!\n");
	return -1;
    }
    
    if( FALSE == DeviceIoControl( h_device, 
				 IOCTL_READ_PORT_WORD,
				 &ioBuf,
				 sizeof(ioBuf),
				 &ioBuf,
				 sizeof(ioBuf),
				 &bytes_returned,
				 NULL))
    {
	display_error_message(GetLastError());
	return -1;
    }

#ifdef VEBOSE_DEBUG_MESSAGE
    printf("number of bytes returned from kernel (IOCTL_READ_PORT_WORD) : %d\n", 
	    bytes_returned);
#endif //VEBOSE_DEBUG_MESSAGE

    return ioBuf.value16;
}


void outw(unsigned short value, unsigned short port)
{
    IO_WORD ioBuf;
    unsigned long bytes_returned;
    
    ioBuf.port16 = port; // port address to write to
    ioBuf.value16 = value; 

    if( INVALID_HANDLE_VALUE == h_device) {
	printf("(outw) Error: the driver handle is invalid!\n");
	return;
    }
    
    if( FALSE == DeviceIoControl( h_device, 
				 IOCTL_WRITE_PORT_WORD,
				 &ioBuf,
				 sizeof(ioBuf),
				 &ioBuf,
				 sizeof(ioBuf),
				 &bytes_returned,
				 NULL))
    {
	display_error_message(GetLastError());
    }

#ifdef VEBOSE_DEBUG_MESSAGE
    printf("number of bytes returned from kernel (IOCTL_WRITE_PORT_WORD) : %d\n", 
	    bytes_returned);
#endif //VEBOSE_DEBUG_MESSAGE
}


unsigned long inl(unsigned short port)
{
    IO_LONG ioBuf;
    unsigned long bytes_returned;
    
    ioBuf.port32 = port; // port address to read from
    

    if( INVALID_HANDLE_VALUE == h_device) {
	printf("(inl) Error: the driver handle is invalid!\n");
	return -1;
    }
    
    if( FALSE == DeviceIoControl( h_device, 
				 IOCTL_READ_PORT_LONG,
				 &ioBuf,
				 sizeof(ioBuf),
				 &ioBuf,
				 sizeof(ioBuf),
				 &bytes_returned,
				 NULL))
    {
	display_error_message(GetLastError());
	return -1;
    }

	 #ifdef VEBOSE_DEBUG_MESSAGE
    printf("number of bytes returned from kernel (IOCTL_READ_PORT_LONG) : %d\n", 
	    bytes_returned);
	 #endif //VEBOSE_DEBUG_MESSAGE

    return ioBuf.value32;
}


void outl(unsigned long value, unsigned short port)
{
    IO_LONG ioBuf;
    unsigned long bytes_returned;
    
    ioBuf.port32 = port; // port address to write to
    ioBuf.value32 = value; 

    if( INVALID_HANDLE_VALUE == h_device) {
	printf("(outl) Error: the driver handle is invalid!\n");
	return;
    }
    
    if( FALSE == DeviceIoControl( h_device, 
				 IOCTL_WRITE_PORT_LONG,
				 &ioBuf,
				 sizeof(ioBuf),
				 &ioBuf,
				 sizeof(ioBuf),
				 &bytes_returned,
				 NULL))
    {
	display_error_message(GetLastError());
    }

 #ifdef VEBOSE_DEBUG_MESSAGE
    printf("number of bytes returned from kernel (IOCTL_WRITE_PORT_LONG) : %d\n", 
	    bytes_returned);
 #endif //VEBOSE_DEBUG_MESSAGE
}


void* map_physical_addr_range( unsigned long phy_addr_start, unsigned long size )
/*++
Routine Description:
	Maps an MMIO range.

Arguments:
    phy_addr_start -  The start of physical address to be mapped.
	size - The size of the MMIO range to map, in bytes.

Return Value:
    NULL - on error
	pointer to the virtual address of the mapped MMIO range - on success

Note:
	-	The pointer returned from this function is in the context of the 
	    usermode application that use this function.
	-	You must call unmap_physical_addr_range when you are done with the  
		mapped physical memory/MMIO range that you obtain with this function.
--*/
{
	MMIO_MAP mmio_map;
	unsigned long bytes_returned;

	if( INVALID_HANDLE_VALUE == h_device) {
		printf("(map_physical_addr_range) Error: the driver handle is invalid!\n");
		return 0; // error
    }
    
	mmio_map.phy_addr_start = phy_addr_start;
	mmio_map.size = size;

    if( FALSE == DeviceIoControl( h_device, 
					IOCTL_MAP_MMIO,
					&mmio_map,
					sizeof(mmio_map),
					&mmio_map,
					sizeof(mmio_map),
					&bytes_returned,
					NULL))
    {
		display_error_message(GetLastError());
		return NULL; // error
    }

 #ifdef VEBOSE_DEBUG_MESSAGE
    printf("number of bytes returned from kernel (IOCTL_MAP_BIOS_CHIP) : %d\n", 
	    bytes_returned);
 #endif //VEBOSE_DEBUG_MESSAGE

    return mmio_map.usermode_virt_addr; 
}	


int unmap_physical_addr_range( void* virt_addr_start, unsigned long size )
/*++
Routine Description:
	Unmaps a previously mapped MMIO range.

Arguments:
    virt_addr_start - The start address of the mapped physical address.
	                This parameter _must_ _be_ in the context of the 
					currently executing application. It must be a 
					pointer returned from a previous call to 
					map_physical_addr_range function.
	size - The size of the mapped MMIO range in bytes.

Return Value:
    0 - on error
	1 - on success

Note:
	This function must be called when you are done with a 
	mapped physical memory/MMIO range.
--*/
{
	MMIO_MAP mmio_map;
	unsigned long bytes_returned;

	if( INVALID_HANDLE_VALUE == h_device) {
		printf("(unmap_physical_addr_range) Error: the driver handle is invalid!\n");
		return 0; // error
    	}
    
	mmio_map.usermode_virt_addr = virt_addr_start;
	mmio_map.size = size;

    if( FALSE == DeviceIoControl( h_device, 
					IOCTL_UNMAP_MMIO,
					&mmio_map,
					sizeof(mmio_map),
					&mmio_map,
					sizeof(mmio_map),
					&bytes_returned,
					NULL))
    {
		display_error_message(GetLastError());
		return 0; // error
    }

 #ifdef VEBOSE_DEBUG_MESSAGE
    printf("number of bytes returned from kernel (IOCTL_UNMAP_BIOS_CHIP) : %d\n", 
	    bytes_returned);
 #endif //VEBOSE_DEBUG_MESSAGE

	return 1; // success
}

