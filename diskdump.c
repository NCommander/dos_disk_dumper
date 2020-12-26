#include <i86.h>
#include <stdio.h>
#include <stdlib.h>
#include <dos.h>
#include <conio.h>
#include <bios.h>
#include <malloc.h>
#include <string.h>
#include <ctype.h>
#include <io.h>

#include "diskdump.h"

// Borrowed from watcom test library
#define HI( w )     (((w) >> 8) & 0xFF)

/* Statically allocate a buffer for data read from the drive */
unsigned char drive_buffer[63*BYTES_PER_SECTOR]; // Hold 16 sectors

#define SECTORS_TO_ALLOCATE 16

int main(int argc, char *argv[]) {
    char file_path[256];
    unsigned int disk_id = 0x80;
    unsigned int cylinders = 0;
    unsigned int heads = 0;
    unsigned int sectors = 0;
    unsigned int num_of_disks = 0;
    unsigned int logical_disk_id = 0;
    float megabytes = 0;
    int ret = 0;
    float megabytes_per_cylinder = 0;
    float cylinder_mb = 0;
    struct diskinfo_t di;
    unsigned short status;
    unsigned int current_cylinder = 0;
    unsigned int current_head = 0;
    unsigned int current_sector = 1;
    int offset = 0;
    int i = 0;
    unsigned int current_drive;
    char drive_letter;
    char letter;
    FILE * dump_file;

    printf("Disk Dumper v0.1\n");
    printf("Copyright (C) 2020 - Michael Casadevall\n");
    printf("\n");

    ret = get_disk_geometry(disk_id+num_of_disks, &cylinders, &heads, &sectors, &num_of_disks);
    /* Loop through HDDs until we find the "right" one */
    printf("Disk Geometry Information:\n");

    disk_id = 0x80;
    for (i = 0 ; i != num_of_disks ; i++) {
        ret = get_disk_geometry(disk_id+i, &cylinders, &heads, &sectors, &num_of_disks);
        if (ret != 0) {
            /* If there's no HDDs, bail */
            if (num_of_disks == 0) {
                printf("BIOS reported error reading disk geometry for disk_id: %x\n", ret);
                return -1;
            }

            // Otherwise we're good!
            break;
        }

        megabytes_per_cylinder = (float)((sectors)*512)/1024;
        cylinder_mb = megabytes_per_cylinder * (float)(cylinders+1)/1024;
        megabytes = cylinder_mb * (heads+1);
        printf("\tDisk %u: Cylinders: %u, Heads: %u, Sectors: %u (%.2f MiB)\n", i+1, cylinders, heads, sectors, megabytes);
    }

    printf("BIOS reported %d fixed disks\n", num_of_disks);
    printf("\n");

    printf("Enter Disk ID to Dump: ");
    scanf("%d", &logical_disk_id);

    if (logical_disk_id == 0 || logical_disk_id > num_of_disks) {
        /* Yeah no */
        printf("ERROR: Invalid Disk ID!");
        return -1;
    }

    // There's a newline in the buffer >.>
    fflush(stdin);

    printf("Enter path to write disk dump to (must be absolute): ");
    fgets(file_path, 255, stdin);

    // fgets captures the newline, so get rid of it
    if (strlen(file_path )!= 1) {
        file_path[strlen(file_path)-1] = '\0';
    } else {
        printf("ERROR: Must enter filepath");
        return -1;
    }

    /**
     * DOS doesn't have realpath, so this isn't quite right, but fuck it,
     * the user has to confirm the path, so YOLO!
     */

    for ( i = 0; i != strlen(file_path); i++) {
        file_path[i] = toupper(file_path[i]);
    }

    // We assume it's absolute path if the second character is :
    if (file_path[1] != ':') {
        printf("ERROR: Cowardly refusing to write to relative path!\n");
        return -1;
    }

    /* Prep for dump */
    disk_id = 0x80+logical_disk_id-1;

    ret = get_disk_geometry(disk_id, &cylinders, &heads, &sectors, &num_of_disks);
    megabytes_per_cylinder = (float)((sectors)*512)/1024;
    cylinder_mb = megabytes_per_cylinder * (float)(cylinders+1)/1024;
    megabytes = cylinder_mb * (heads+1);

    if (ret != 0) {
        printf("BIOS reported error reading disk geometry for disk_id: %x\n", ret);
        return -1;
    }

    printf("\n");
    printf("I'm going to dump this disk:\n");
    printf("\tDisk %u: Cylinders: %u, Heads: %u, Sectors: %u (%.2f MiB)\n", logical_disk_id, cylinders, heads, sectors, megabytes);
    printf("\t(Dump file will be %lld bytes)\n", (long long)sectors*512*(cylinders+1)*(heads+1));
    printf("\nAnd I'm going to write it here:\n");
    printf("\t%s\n\n", file_path);
    printf("Are you sure? (y/n) ");

    letter = getchar();
    if (letter != 'y') {
        printf("Aborted.\n");
        return 0;
    }

    if (getchar() != '\n') { /* make sure we just typed y */
        printf("Aborted.\n");
        return 0;
    }

    dump_file = fopen(file_path, "wb");
    if (dump_file == 0) {
        perror("fopen failed with");
        return -1;
    }

    /* Do a reset on the drive to make sure it's ready */
    di.drive = disk_id;
    di.head = 0;
    di.track = 0;
    di.sector = 1;
    di.nsectors = 1;
    di.buffer = NULL;

    printf("\n");

    // Not noted in the docs, but error is in the high bit
    status = _bios_disk(_DISK_RESET, &di);
    if (HI(status) != 0) {
        printf("_DISK_RESET failed with %4.4x! Is hard drive actually ready?\n", status);
        ret = 1;
        goto clean_up;
    }

    for (current_cylinder = 0; current_cylinder != cylinders+1; current_cylinder++) {
        for (current_head = 0; current_head != heads+1; current_head++) {
            for (current_sector = 1;  current_sector != sectors+1;) {
                int nsectors = 16;

                // Read the buffer until it's full, or we run out of sectors to read
                di.drive = disk_id;
                di.track = current_cylinder;
                di.head = current_head;
                di.sector = current_sector;
                di.buffer = drive_buffer+offset;

                if (current_sector + 16 > sectors) {
                    nsectors = sectors - current_sector + 1;
                }

                di.nsectors = nsectors;

                printf("Reading Cylinder: %4d, Head: %2d (Progress: %2.2f%%)\r",
                    current_cylinder,
                    current_head,
                    ((float)current_cylinder/cylinders)*100);

                for (i = 0; i != 3; i++) {
                    status = _bios_disk(_DISK_READ, &di);
                    if (HI(status) != 0) {
                        printf("_DISK_READ failed with 0x%2.2x! Retry: %d\n", HI(status), i+1);
                        sleep(1);
                    } else {
                        // Read the data successfully
                        break;
                    }
                }

                current_sector += nsectors;

                if (HI(status) != 0) {
                    printf("Hit retry count, bailing out\n");
                    goto clean_up;
                }


                // Write the data out
                printf("Writing Cylinder: %4d, Head: %2d (Progress: %2.2f%%)\r",
                    current_cylinder,
                    current_head,
                     ((float)current_cylinder/cylinders)*100);

                ret = fwrite((void*)drive_buffer, 1, BYTES_PER_SECTOR*nsectors, dump_file);
                if (ret != BYTES_PER_SECTOR*nsectors) {
                    printf("ERROR: fwrite short wrote %d, aborting\n", ret);
                    goto clean_up1;
                }
            }
        }
    }


    printf("\nDump Complete!");

    clean_up1:
    fclose(dump_file);

    clean_up:
    printf("\n");
    return ret;
}
