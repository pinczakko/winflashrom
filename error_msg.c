#include <windows.h>
#include <stdio.h>
#include "error_msg.h"

VOID display_error_message(DWORD error_number)
{
	
    LPVOID lpMsgBuf;
    if (!FormatMessage( 
		FORMAT_MESSAGE_ALLOCATE_BUFFER | 
        FORMAT_MESSAGE_FROM_SYSTEM | 
		FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
		error_number,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
		(LPTSTR) &lpMsgBuf,
        0,
		NULL ))
    {
	// Handle the error.
	return;
    }

    // Display the string.
    printf("Error! %s\n", (LPCTSTR)lpMsgBuf );

    // Free the buffer.
    LocalFree( lpMsgBuf );
}

