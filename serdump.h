#ifndef _SERDUMP_H_
#define _SERDUMP_H_

#include "crc16.h"

#define BYTES_PER_SECTOR 512

int __cdecl get_disk_geometry(unsigned int disk_id, unsigned int *cylinders, unsigned int *heads, unsigned int *sectors, unsigned int *num_of_disks);

// Transmit something over xmodem
int xmodemTransmit(unsigned char *src, int srcsz);

int _inbyte(unsigned short timeout); // msec timeout
void _outbyte(int c);

#endif // _SERDUMP_H_
