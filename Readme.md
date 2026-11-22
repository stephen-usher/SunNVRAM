SUNNVRAM - Creates an image of a Sun NVRAM ready to be writeen to a M48T02.

This program creates a 2K byte image file (by default nvram.bin) containing the settings and IDPROM contents for a Sun 3/80 or first generation SPARCstation.

Without a configuration file (sunnvram.cfg) the defaults are for my Sun 3/80 booting into diagnostic mode.

The image can then be written into a M48T02 chip using an Xgecu EPROM programmer, such as the TL866I or T48.
(Data verify should be turned off as the clock will cause the test to fail.)

The minimal, meaningful configuration file should contain the machine type and the serial number.
Everything else is optional as you can get to the command prompt with thse set and continue from there.
The MAC address and HOSTID will be set using the serial number information.

This program is especially useful for the Sun 3/80 as the boot ROM doesn't set sane values and it's difficult
to enter the values in from the command line. The SPARCstation boot ROMs are far easier with the Openboot FORTH
interpreter in (n)ew mode.

The program should be easily augmented to add the later SPARCstations.

To build on UNIX/Linux machines just run 'make'.

Stephen Usher, 22nd November, 2026.
