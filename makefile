# OpenWatcom Makefile, use wmake

DOS_H = $(%WATCOM)/h

C16FLAGS  = -i=$(DOS_H) -bt=dos -ms -q -ec
WC16    = wcc $(C16FLAGS)
ASM = wasm
AFLAGS = -zq -0

LNKOPT = option quiet

.EXTENSIONS:
.EXTENSIONS: .exe .rex .lib .obj .wbj .asm .c .for

all:	.SYMBOLIC serdump.exe

HELLOOBJS = serdump.obj utils.obj xmodem.obj crc16.obj serialio.obj com.obj
serdump.exe: $(HELLOOBJS) .PRECIOUS
	wlink $(LNKOPT) system dos name serdump &
                file serdump.obj &
                file utils.obj &
                file crc16.obj &
                file xmodem.obj &
                file serialio.obj &
                file com.obj

clean: .SYMBOLIC
        @if exist *.obj rm *.obj
        @if exist *.exe rm *.exe
        @if exist *.sys rm *.sys
        @if exist *.err rm *.err
        @if exist *.map rm *.map
        @if exist *.lnk rm *.lnk

.c.obj: .AUTODEPEND
        $(WC16) $(C16FLAGS) -fo=$*.obj $*.c 

.asm.obj : .AUTODEPEND
        $(ASM) $(AFLAGS) -fo=$*.obj $^&.asm
		
