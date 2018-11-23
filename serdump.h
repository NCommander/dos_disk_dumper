#ifndef _SERDUMP_H_
#define _SERDUMP_H_

#include "crc16.h"
#include "com.h"
#define BYTES_PER_SECTOR 512

int __cdecl get_disk_geometry(unsigned int disk_id, unsigned int *cylinders, unsigned int *heads, unsigned int *sectors, unsigned int *num_of_disks);
void __cdecl init_serial_port(int port, int divisor);

// Transmit something over xmodem
int xmodemTransmit(unsigned char __far *src, int srcsz, unsigned int * packetno);
void xmodemFinalize();

int _inbyte(unsigned short timeout); // msec timeout
void _outbyte(unsigned char c);

#endif // _SERDUMP_H_
