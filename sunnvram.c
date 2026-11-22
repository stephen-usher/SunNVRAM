/*

	sunnvram.c

	Creates an image of the Sun SPARCstation NVRAM ready
	to be written to an M48T02 using an Xgecu EPROM
	writing device.
	(Turn off verify as clock will cause failure.)

	The default values are for my Sun 3/80.

	(c) Stephen Usher, 2026.

	Licensed as Creative Commons Attribution.

	Version 1.0: Supports the Sun 3/80 and 1st Gen. SPARCstations.
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <ctype.h>

#define NVRAMLEN 2048
#define CFGFILE  "sunnvram.cfg"

void parse_config_file();
void parse_machtype(char *val);
void parse_sernum(char *val);
void parse_bootdev(char *val);
int parse_numval(char *val);
int parse_yesno(char *val);
int my_readline(FILE *fp, char *buffer, int maxsize);

unsigned char nvram[NVRAMLEN];

char *filename;

int main(int argc, char **argv)
{
	int i, checksum, fd;

	filename = "nvram.bin";

	for (i = 0; i < NVRAMLEN; i++)
		nvram[i] = 0;
		
	nvram[0x14] = 16;	/* Installed RAM */
	nvram[0x15] = 1;	/* RAM tested */
	nvram[0x16] = 0;	/* Standard resolution */
	nvram[0x17] = 0;	/* Watchdog go to monitor */
	nvram[0x18] = 0;	/* Boot device polled */
	nvram[0x19] = 's';
	nvram[0x1a] = 'd';
	nvram[0x1b] = 0;	/* Bus 0 */
	nvram[0x1c] = 0;	/* Target 0 */
	nvram[0x1d] = 0;	/* Partition 0 */
	nvram[0x1f] = 0x0;	/* Primary terminal ttya */
	nvram[0x21] = 0;	/* No keyboard click */

	nvram[0x70b] = 0x12;	/* Diag mode */

/* IDPROM stuff */

	nvram[0x7d8] = 0x01;
	nvram[0x7d9] = 0x42;	/* Sun 3/80 */

/*
 * Sun Microsystems MAC address head
 */

	nvram[0x7da] = 0x08;
	nvram[0x7db] = 0x00;
	nvram[0x7dc] = 0x20;

/*
 * The rest of the MAC address, the last three values
 *  from the serial number.
 */

	nvram[0x7dd] = 0x0f;
	nvram[0x7de] = 0x29;
	nvram[0x7df] = 0x97;

/*
 * Manufacture date
 */
	nvram[0x7e0] = 0;
	nvram[0x7e1] = 0;
	nvram[0x7e2] = 0;

/*
 * The last four values of the serial number
 */
	nvram[0x7e3] = 0x93;
	nvram[0x7e4] = 0x0f;
	nvram[0x7e5] = 0x29;
	nvram[0x7e6] = 0x97;

// Now that the default values are set up see if they need changing.

	parse_config_file();

// Do the IDPROM checksum.

	checksum = 0;

	for (i = 0x7d8; i < 0x7e7; i++)
	{
		checksum ^= nvram[i];
	}

	nvram[0x7e7] = checksum;

// Open the output file.

	if ((fd = open(filename, O_CREAT|O_TRUNC|O_RDWR)) == -1)
	{
		perror(filename);
		exit(-1);
	}

	write(fd, nvram, NVRAMLEN);

	close(fd);
}

#define MAXLINELEN 1024
char buffer[MAXLINELEN];

int numcmds = 10;

#define VAR_MACHTYPE	0
#define VAR_SERNUM	1
#define VAR_DIAGSW	2
#define VAR_RAMTSTSZ	3
#define VAR_BOOTDEV	4
#define VAR_BOOTBUS	5
#define VAR_BOOTTARGET	6
#define VAR_BOOTPART	7
#define VAR_KBDCLK	8
#define VAR_OUTPUTFILE	9

char *values[10] = {
	"machine_type",
	"serial_number",
	"diag_switch",
	"test_ram",
	"boot_dev",
	"boot_bus",
	"boot_target",
	"boot_partition",
	"keyboard_click",
	"output_file"
};

char output_filename[MAXLINELEN];

int lineno;

void parse_config_file()
{
	FILE *fp;
	int linelen, i, j, curcmd;
	char *value, *equals;
	
	lineno = 1;

	if ((fp = fopen(CFGFILE,"r")) == NULL)
	{
		printf("Config file %s not found, using defaults.\n", CFGFILE);
		return;
	}

	while(my_readline(fp, buffer, MAXLINELEN) != -1)
	{
		linelen = strnlen(buffer, MAXLINELEN);

		if (linelen == 0)
			continue;

		if (buffer[0] == '#')
			continue;

// Lowercase all characters and remove spaces and tabs

		for (i = 0; i < linelen; i++)
		{
			if ((buffer[i] >= 'A') && (buffer[i] <= 'Z'))
				buffer[i] = buffer[i] + 0x20;

			if (buffer[i] == ' ')
			{
				for (j = i + 1; j <= linelen ; j++)
					buffer[j - 1] = buffer[j];

				linelen = linelen - 1;
			}

			if (buffer[i] == '\t')
			{

				for (j = i + 1; j <= linelen ; j++)
					buffer[j - 1] = buffer[j];

				linelen = linelen - 1;
			}
		}


// Split line at first '=' character

		if ((equals = strchr(buffer, '=')) == NULL)
		{
			printf("%s: Invalid syntax at line %d\n", CFGFILE, lineno);
			continue;
		}
		else
		{
			*equals = '\0';
			value = equals + 1;
		}

// Try to find which option is being set

		curcmd = -1;

		for (j = 0; j < numcmds; j++)
		{
			if (strncmp(buffer, values[j], linelen) == 0)
			{
				curcmd = j;
			}
		}

		if (curcmd == -1)
		{
			printf("%s: Unknown option '%s' at line %d.\n", CFGFILE, buffer, lineno);
			continue;
		}

		switch(curcmd)
		{
			case VAR_MACHTYPE:
				parse_machtype(value);
				break;
			case VAR_SERNUM:
				parse_sernum(value);
				break;
			case VAR_DIAGSW:
				j = parse_yesno(value);
				if (j == 1)
					nvram[0x70b] = 0x12;
				else
					nvram[0x70b] = 0;
				break;
			case VAR_RAMTSTSZ:
				nvram[0x15] = parse_numval(value);
				break;
			case VAR_BOOTDEV:
				parse_bootdev(value);
				break;
			case VAR_BOOTBUS:
				nvram[0x1b] = parse_numval(value);
				break;
			case VAR_BOOTTARGET:
				nvram[0x1c] = parse_numval(value);
				break;
			case VAR_BOOTPART:
				nvram[0x1d] = parse_numval(value);
				break;
			case VAR_KBDCLK:
				nvram[0x21] = parse_yesno(value);
				break;
			case VAR_OUTPUTFILE:
				strncpy(output_filename, value, linelen);
				filename = output_filename;
				break;
		}

		lineno++;
	}

	fclose(fp);
}

#define NUMMACHINES 11

char *mach_names[NUMMACHINES] = {
	"sun380",				/* 3/80 */
	"ss1",					/* SS1 */
	"sparcstation1",			/* SS1 */
	"ss1+",					/* SS1+ */
	"sparcstation1+",			/* SS1+ */
	"ss2",					/* SS2 */
	"sparcstation2",			/* SS2 */
	"ipc",					/* IPC */
	"ipx",					/* IPX */
	"slc",					/* SLC */
	"elc"					/* ELC */
};

unsigned char mach_codes[NUMMACHINES] = {
	0x42,					/* 3/80 */
	0x51,					/* SS1 */
	0x51,					/* SS1 */
	0x53,					/* SS1+ */
	0x53,					/* SS1+ */
	0x55,					/* SS2 */
	0x55,					/* SS2 */
	0x52,					/* IPC */
	0x57,					/* IPX */
	0x54,					/* SLC */
	0x56					/* ELC */
};

void parse_machtype(char *val)
{
	int i;
	int found = 0;

	for (i = 0; i < NUMMACHINES; i++)
	{
		if (strncmp(val, mach_names[i], 20) == 0)
		{
			nvram[0x7d9] = mach_codes[i];
			found = 1;
			break;
		}
	}

	if (found == 0)
		printf("Machine type '%s' not found in list.\n", val);
}

void parse_sernum(char *val)
{
	int i, length, whichbyte;
	char *ptr;
	int nibble;
	unsigned char bytevals[4] = { 0, 0, 0, 0};

	length = strnlen(val ,20);

	if (length < 8)
	{
		printf("Invalid serial number length, too short.\n");
		return;
	}

// Just look at the last 8 characters of the serial number.

	ptr = val + length - 8;

	for (i = 0; i < 8; i++)
	{
		whichbyte = i / 2;

		if (!isalnum(ptr[i]))
		{
			printf("Invalid characters in serial number.\n");
			return;
		}

		if (ptr[i] > 'f')
		{
			printf("Invalid characters in serial number.\n");
			return;
		}

		if (ptr[i] >= 'a')
		{
			nibble = 9 + (ptr[i] - 'a');
		}
		else
		{
			nibble = ptr[i] - '0';
		}

		if (i - (whichbyte * 2) == 0)
			nibble = nibble * 16;

		bytevals[whichbyte] += nibble;
	}

// Setup MAC address.

	nvram[0x7dd] = bytevals[1];
	nvram[0x7de] = bytevals[2];
	nvram[0x7df] = bytevals[3];

// Setup Serial number.
	nvram[0x7e3] = bytevals[0];
	nvram[0x7e4] = bytevals[1];
	nvram[0x7e5] = bytevals[2];
	nvram[0x7e6] = bytevals[3];
}

void parse_bootdev(char *val)
{
// We've specified a boot device so not polled.
	nvram[0x18] = 1;

// Copy the first two characters into the NVRAM template.

	nvram[0x19] = val[0];
	nvram[0x1a] = val[1];
}

int parse_numval(char *val)
{
	int retval = 0;

	retval = atoi(val);
	
	return retval;
}

int parse_yesno(char *val)
{
	int retval = 0;

// Anything other than 'yes' or '1' is no

	if ((strncmp(val, "yes", 3) == 0) || (val[0] == '1'))
		retval = 1;
	
	return retval;
}

int my_readline(FILE *fp, char *buffer, int maxsize)
{
	int cur = 0, ret;
	char value;
	
	while(((ret = fread(&value, 1, 1, fp)) != 0) && (value != '\n') && (cur < maxsize - 1))
	{
		buffer[cur++] = value;
	}

	buffer[cur] = '\0';

	if (ret == 0)
		return -1;
	else
		return cur;
}
