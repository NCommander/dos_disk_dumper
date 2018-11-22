#include <stdio.h>
#include <stdlib.h>
#include <dos.h>
#include <conio.h>

#include "com.h"
#include "serdump.h"

void __cdecl __far ack_pic();

int main(int argc, char *argv[]) {
    unsigned int disk_id = 0x80;
    unsigned int cylinders = 10;
    unsigned int heads = 0;
    unsigned int sectors = 0;
    unsigned int num_of_disks = 0;
    float megabytes = 0;
    int ret = 0;
    float megabytes_per_cylinder = 0;
    float cylinder_mb = 0;
    PORT *port;
    int c;

    printf("Serial Dumper v0.1\n");
    printf("Copyright (C) 2018 - Michael Casadevall\n");
    printf("\n");

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
    port_set( port, 2400L, 'N', 8, 1 );
    /*
    * The program stays in this loop until the user hits the
    * Escape key.  The loop reads in a character from the
    * keyboard and sends it to the COM port.  It then reads
    * in a character from the COM port, and prints it on the
    * screen.
    */
    for ( ; ; ) {
        if ( kbhit() ) {
            c = getch();
            if ( c == 27 )
                break;
            else
                port_putc( (unsigned char) c, port );
        }
        c = port_getc( port );
        if ( c >= 0 )
            putc( c, stdout );
    }
    port_close( port );
    return 0;

}
