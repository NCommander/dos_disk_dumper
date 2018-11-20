#include <stdio.h>

void asm_test(int *value1, int* value2);
int __cdecl get_disk_geometry(unsigned int disk_id, unsigned int *cylinders, unsigned int *heads, unsigned int *sectors, unsigned int *num_of_disks);

#define BYTES_PER_SECTOR 512
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

    return 0;
}
