#include <i86.h>
#include <stdio.h>
#include <stdlib.h>
#include <dos.h>
#include <conio.h>
#include <bios.h>
#include <malloc.h>

#include "com.h"
#include "serdump.h"

// XModem support features
PORT *port;
int c;

// Borrowed from watcom test library
#define HI( w )     (((w) >> 8) & 0xFF)

/* Statically allocate a buffer for data read from the drive */
unsigned char __far drive_buffer[16*BYTES_PER_SECTOR]; // Hold 16 sectors

// Send a bute out on serial
void _outbyte(unsigned char c) {
    int ret = port_putc(c, port);

    while (ret == -1) {
        delay(1);
        ret = port_putc(c, port);
    };
}

// Read byte in
int _inbyte(unsigned short timeout) {
    int c;

     do {
        c=port_getc(port);
        if (c == -1) {
            delay(1); /* 60 us * 16 = 960 us (~ 1 ms) */
        }
        if (timeout) {
            if (--timeout == 0) {
                printf("Timeout");
                return -2;
            }
        }
    } while (c == -1);

    return (unsigned short)c;
}

#define SECTORS_TO_ALLOCATE 16
#define DRIVE_ID 0x80
int main(int argc, char *argv[]) {
    unsigned int disk_id = DRIVE_ID;
    unsigned int cylinders = 0;
    unsigned int heads = 0;
    unsigned int sectors = 0;
    unsigned int num_of_disks = 0;
    float megabytes = 0;
    int ret = 0;
    float megabytes_per_cylinder = 0;
    float cylinder_mb = 0;
    struct diskinfo_t di;
    unsigned short status;
    unsigned int current_cylinder = 0;
    unsigned int current_head = 0;
    unsigned int current_sector = 1;
    unsigned int packet_number = 1;
    int offset = 0;
    int i;

    printf("Serial Dumper v0.1\n");
    printf("Copyright (C) 2018 - Michael Casadevall\n");
    printf("\n");

    if (drive_buffer == NULL) {
        printf("ERROR, unable to allocate %d bytes\n, erroring out!", SECTORS_TO_ALLOCATE*BYTES_PER_SECTOR);
    }

    ret = get_disk_geometry(disk_id, &cylinders, &heads, &sectors, &num_of_disks);
    if (ret != 1) {
        printf("BIOS reported error reading disk geometry for disk_id: %x\n", ret);
        printf("%d\n", cylinders);
        return -1;
    }

    megabytes_per_cylinder = (float)((sectors)*512)/1024;
    cylinder_mb = megabytes_per_cylinder * (float)cylinders/1024;
    megabytes = cylinder_mb * (heads+1);
    printf("Disk Geometry Information:\n");
    printf("\tDisk %u: Cylinders: %u, Heads: %u, Sectors: %u (%.2f MiB)\n\n ", disk_id-0x80+1, cylinders, heads+1, sectors, megabytes);

    printf("BIOS reported %d fixed disks\n", num_of_disks);

    /* Initialize the serial port and install handler */
    port = port_open( COM1_UART, COM1_INTERRUPT );
    if ( port== NULL ) {
        printf( "Failed to open the port!\n" );
        exit( 1 );
    }
    port_set( port, 115200L, 'N', 8, 1 );

    /* Start reading sectors in from the drive one by one */
    printf("\n");

    /* Do a reset on the drive to make sure it's ready */
    di.drive = DRIVE_ID;
    di.head = 0;
    di.track = 0;
    di.sector = 1;
    di.nsectors = 1;
    di.buffer = NULL;

    // Not noted in the docs, but error is in the high bit
    status = _bios_disk(_DISK_RESET, &di);
    if (HI(status) != 0) {
        printf("_DISK_RESET failed with %4.4x! Is hard drive actually ready?\n", status);
        ret = 1;
        goto clean_up;
    }

    /* Start reading sectors, docs say that we should try up to three times to read a sector in case of error */
    for (current_cylinder = 0; current_cylinder != cylinders+1; current_cylinder++) {
        for (current_head = 0; current_head != heads+1; current_head++) {
            offset = 0; /* Go back to the start of the buffer for each head change */
            for (current_sector = 1; current_sector != sectors+1; current_sector++) {
                // Read the buffer until it's full, or we run out of sectors to read
                di.drive = DRIVE_ID;
                di.track = current_cylinder;
                di.head = current_head;
                di.sector = current_sector;
                di.nsectors = 1;
                di.buffer = drive_buffer+offset;

                printf("Reading Cylinder: %d, Head: %d, Sector: %d\n", current_cylinder, current_head, current_sector);
                for (i = 0; i != 3; i++) {
                    status = _bios_disk(_DISK_READ, &di);
                    if (HI(status) != 0) {
                        printf("_DISK_READ failed with %4.4x! Retry: %d\n", status, i);
                    } else {
                        // Read the data successfully
                        break;
                    }
                }

                if (HI(status) != 0) {
                    printf("Hit retry count, bailing out\n");
                    goto clean_up;
                }

                // xmodemTransmit must always be a multiple of 128
                printf("Transmitting Cylinder: %d, Head: %d, Sector: %d\n", current_cylinder, current_head, current_sector);
                for (i = 0; i !=4 ; i++) { 
                    int offset2 = i*128;
                    status = xmodemTransmit(drive_buffer+offset+offset2, 128, &packet_number);
                    if (packet_number < 0) {
                        printf("ERROR: Transmission Error %l\n", packet_number);
                        goto clean_up;
                    }
                }

                offset = offset + BYTES_PER_SECTOR;
                // If we're reached the end of the buffer, reset it to zero and keep going
                if (offset >= 15*BYTES_PER_SECTOR) {
                    offset = 0;
                }

                // If the buffer is fill, transmit it, and then keep going
            }
        }
    }

    xmodemFinalize();

    clean_up:
    port_close(port);
    return ret;
}
