#ifndef _DISKDUMP_H_
#define _DISKDUMP_H_

#define BYTES_PER_SECTOR 512

int __cdecl get_disk_geometry(unsigned int disk_id, unsigned int *cylinders, unsigned int *heads, unsigned int *sectors, unsigned int *num_of_disks);

#endif // _DISKDUMP_H_
