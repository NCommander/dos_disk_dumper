# OpenWatcom Makefile, use wmake

DOS_H = $(%WATCOM)/h

C16FLAGS  = -i=$(DOS_H) -bt=dos -ms -q
WC16    = wcc $(C16FLAGS)

.EXTENSIONS:
.EXTENSIONS: .exe .rex .lib .o .wbj .asm .c .for
.c.o: .AUTODEPEND
        $(WC16) $(C16FLAGS) $*.c

all:	.SYMBOLIC hello.exe

HELLOOBJS = hello.o
hello.exe: $(HELLOOBJS) .PRECIOUS
	wlink $(LNKOPT) system dos name hello file hello

clean: .SYMBOLIC
        @if exist *.o rm *.o
        @if exist *.exe rm *.exe
        @if exist *.sys rm *.sys
        @if exist *.err rm *.err
        @if exist *.map rm *.map
        @if exist *.lnk rm *.lnk