-------------------------------------------------------------------------------
Winflashrom README
-------------------------------------------------------------------------------

This is the Windows port of the universal (LinuxBIOS) flash utility.
It's named winflashrom. This code has not been merged into the LinuxBIOS 
repository. Therefore, it's available publicly only from Google Project 
hosting for a while. There are still some minor issues that prevent us 
to merge it into the LinuxBIOS repository. Anyway, you might want to 
watch LinuxBIOS mailing list 
(http://www.linuxbios.org/mailman/listinfo/linuxbios)
for further updates on Winflashrom. 
Once a stable release of Winflashrom is available. The code in 
Google Project hosting will be updated as well.


Build Requirements
------------------

To build the winflashrom utility you need to have the following packages
installed on your Windows system:

* MinGW
* MSys
* Windows XP DDK or Windows Driver Kit (WDK). This tool is optional because the
  package comes with a precompiled driver. However, if you want to make changes 
  to the driver source code, you will need one of them.
* DbgView (optional). You can use this tool for remote debugging if you want to 
  try some changes to the driver code. DbgView is available from Sysinternals as 
  a freeware.

How to build winflashrom from the source code
----------------------------------------------
* _Building the application_. 
  To build the application, "cd" to the source code root directory of the source code 
  from within MSys (or other compatible shell that you might use) and invoke "make".
  
* _Building the driver_. 
  To build the driver, run the Windows XP DKK or WDK shell (The WinXP Free Build 
  environment or the WinXP Checked Build environment). Then, "cd" to the directory 
  named "driver" in the root directory of the source code and invoke "build" from 
  there. If you would like to have debugging message, use the "The WinXP checked 
  build environment" of the Windows XP DDK or WDK shell.

* _Remote debugging the driver_. 
  To do a remote debugging, first run the DbgView client in the _test machine_ by invoking 
  the following command in DbgView installation directory: `DbgView.exe /c /t`
  Then, connect to the test machine from the _remote machine_ by running `DbgView.exe`.
  In the remote DbgView window, activate the Capture|Capture Kernel option, 
  Capture|Pass-Through option, and Capture|Capture Events option and 
  Disable the Capture|Capture Wind32 option as it will clutter the output with 
  unnecessary messages. Now, you can monitor the debug messages from the _test machine_.
  


Usage
-----

usage: 

    ./flashrom [-rwvEVfh] [-c chipname] [-s exclude_start]
    [-e exclude_end] [-m vendor:part] [-l file.layout] [-i imagename] [file]
    -r | --read:                    read flash and save into file
    -w | --write:                   write file into flash (default when
                                    file is specified)
    -v | --verify:                  verify flash against file
    -E | --erase:                   erase flash device
    -V | --verbose:                 more verbose output
    -c | --chip <chipname>:         probe only for specified flash chip
    -s | --estart <addr>:           exclude start position
    -e | --eend <addr>:             exclude end postion
    -m | --mainboard <vendor:part>: override mainboard settings
    -f | --force:                   force write without checking image
    -l | --layout <file.layout>:    read rom layout from file
    -i | --image <name>:            only flash image name from flash layout

 If no file is specified, then all that happens
 is that flash info is dumped and the flash chip is set to writable.


LinuxBIOS Table and Mainboard Identification
--------------------------------------------

Flashrom reads the LinuxBIOS table to determine the current mainboard.
(Parse DMI as well in future?) If no LinuxBIOS table could be read
or if you want to override these values, you can specify -m, e.g.:

    flashrom -w --mainboard ISLAND:ARUMA island_aruma.rom

The following boards require the specification of the board name, if
no LinuxBIOS table is found:

* IWILL DK8-HTX: use `-m iwill:dk8_htx`
* Agami Aruma: use `-m AGAMI:ARUMA`
* ASUS P5A: use `-m asus:p5a`
* IBM x3455: use `-m ibm:x3455`
* EPoX EP-BX3: use `-m epox:ep-bx3`


ROM Layout Support
------------------

Flashrom supports ROM layouts. This allows to flash certain parts of
the flash chip only. A ROM layout file looks like follows:

    00000000:00008fff gfxrom
    00009000:0003ffff normal
    00040000:0007ffff fallback
  
  i.e.:
  
    startaddr:endaddr name

  all addresses are offsets within the file, not absolute addresses!
  
If you only want to update the normal image in a ROM you can say:

     flashrom -w --layout rom.layout --image normal island_aruma.rom
     
To update normal and fallback but leave the VGA BIOS alone, say:

     flashrom -w -l rom.layout -i normal -i fallback island_aruma.rom
 
Currently overlapping sections are not supported.

ROM layouts should replace the -s and -e option since they are more 
flexible and they should lead to a ROM update file format with the 
ROM layout and the ROM image in one file (cpio, zip or something?)


DOC support
-----------

DISK on Chip support is currently disabled since it is considered unstable. 
Change `CFLAGS` in the Makefile to enable it: Remove `-DDISABLE_DOC` from `CFLAGS`.


Supported Flash Chips
---------------------

* AMD AM-29F040B
* AMD AM-29F016D
* ASD AE49F2008
* Atmel AT-29C040A
* Atmel AT-29C020
* EMST F49B002UA
* Intel 82802AB (Firmware Hub) 
* Intel 82802AC (Firmware Hub) 
* M-Systems MD-2802 (unsupported, disabled by default)
* MX MX-29F002
* PMC PMC-49FL002
* PMC PMC-49FL004
* Sharp LHF-00L04
* SST SST-29EE020A
* SST SST-28SF040A
* SST SST-39SF010A
* SST SST-39SF020A
* SST SST-39SF040
* SST SST-39VF020
* SST SST-49LF040B
* SST SST-49LF040
* SST SST-49LF020A
* SST SST-49LF080A
* SST SST-49LF160C
* SST SST-49LF002A/B
* SST SST-49LF003A/B
* SST SST-49LF004A/B
* SST SST-49LF008A
* SST SST-49LF004C
* SST SST-49LF008C
* SST SST-49LF016C
* ST ST-M50FLW040A
* ST ST-M50FLW040B
* ST ST-M50FLW080A
* ST ST-M50FLW080B
* ST ST-M50FW040
* ST ST-M50FW080
* ST ST-M50FW016
* ST ST-M50LPW116
* ST ST-M29F002B
* ST ST-M29F002T
* ST ST-M29F002NT
* ST ST-M29F400BT
* ST ST-M29F040B
* ST ST-M29W010B
* ST ST-M29W040B
* SyncMOS S29C51001T/B
* SyncMOS S29C51002T/B
* SyncMOS S29C51004T/B
* SyncMOS S29C31004T
* Winbond W29C011
* Winbond W29C020C
* Winbond W49F002U
* Winbond W49V002A
* Winbond W49V002FA
* Winbond W39V040FA
* Winbond W39V040A
* Winbond W39V040B
* Winbond W39V080A

Supported Southbridges
----------------------
* AMD CS5530/CS5530A
* AMD Geode SC1100
* AMD AMD-8111
* ATI SB400
* Broadcom HT-1000
* Intel ICH0-ICH8 (all variations)
* Intel PIIX4/PIIX4E/PIIX4M
* NVIDIA CK804
* NVIDIA MCP51
* NVIDIA MCP55
* SiS 630
* SiS 5595
* VIA CX700
* VIA VT8231
* VIA VT8235
* VIA VT8237
* VIA VT82C686

Note on the current state of tested platform(s)
-----------------------------------------------
This code has been successfully tested on Pentium 4 
(single core) system with ICH5 chipset and Winbond 
W39V040FA flash chip. 

The driver architecture is still messy at this point.
However, I'm working on to improve it right now and it 
should come with better code very soon.

