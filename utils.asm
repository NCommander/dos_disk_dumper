.8086

; I'm guessing because WASM is hard to find example code for in real mode

SaveRegisters MACRO
    push bx
    push cx
    push dx
    push si
    push di
ENDM

RestoreRegisters MACRO
    pop bx
    pop cx
    pop dx
    pop si
    pop di
ENDM

_TEXT SEGMENT 'CODE'
    ASSUME CS:_TEXT


; Returns the number of HDDs seen by the system
PUBLIC _get_disk_geometry

; get_disk_geometry(int disk_id, int *cylinders, int *heads, int *sectors, int *num_of_disks)
_get_disk_geometry   PROC NEAR
    _disk_id$ = 4
    _cylinders$ = 6
    _heads$ = 8
    _sectors$ = 10
    _num_of_disks$ = 12

    push bp
    mov bp, sp

    SaveRegisters
    
    ; Setup for int 13h call
    ; Start with first fixed disk, we'll work from there
    push es

    xor ax, ax
    xor dx, dx
    mov ah, 08h
    mov dx, 80h

    mov es, ax
    xor di, 0h
    int 13h

    ; if an error was returned in CF
    jc error1

    ; Return number of heads
    mov bx, WORD PTR [bp+_heads$]
    mov WORD PTR [bx], 0
    mov BYTE PTR [bx], dh

    ; Return number of disks
    mov bx, WORD PTR [bp+_num_of_disks$]
    mov WORD PTR [bx], 0
    mov BYTE PTR [bx], dl

    ; Cylinders and sectors share cx, get each value and seperate them out
    push cx ; make a copy
    and cx, 3fh
    mov bx, WORD PTR [bp+_sectors$]
    mov WORD PTR [bx], 0
    mov BYTE PTR [bx], cl

    pop cx
    ; Now get cylinders

    and cl, 0c0h
    shr cl, 1
    shr cl, 1
    shr cl, 1
    shr cl, 1
    shr cl, 1
    shr cl, 1
    xchg ch, cl
    mov bx, WORD PTR [bp+_cylinders$]
    mov WORD PTR[bx], cx

    mov ax, 1
    pop es
    RestoreRegisters
    pop bp
    ret

    error1:
    mov ax, -1
    pop es
    RestoreRegisters
    pop bp
    ret

    _get_disk_geometry ENDP


; init_serial_port(int port_id, int divisor)
; Returns the number of HDDs seen by the system
PUBLIC _init_serial_port

; void init_serial_port(int port, divisor)
_init_serial_port   PROC NEAR
    _port$ = 4
    _divisor$ = 6

    push bp
    mov bp, sp

    SaveRegisters
    ; This is going to be ugly

    ; Disable interrupts
    mov dx, WORD PTR [bp+_port$]
    add dx, 1
    mov al, 0
    out dx, al

    ; We're only going to support writing at 8N1

    ; Enable DLAB
    mov dx, WORD PTR [bp+_port$]
    add dx, 3
    mov al, 80h
    out dx, al

    ; Set divisor (high bit)
    mov dx, WORD PTR [bp+_port$]
    mov bx, WORD PTR [bp+_divisor$]
    mov al, bl
    out dx, al

    ; Set divisor (low bit)
    mov dx, WORD PTR [bp+_port$]
    add dx, 1
    mov al, bh
    out dx, al

    ; Set 8N1
    mov dx, WORD PTR [bp+_port$]
    add dx, 3
    mov al, 03h
    out dx, al

    ; Enable FIFO queue
    mov dx, WORD PTR [bp+_port$]
    add dx, 2
    mov al, 0c7h
    out dx, al

    ; Enable initerrupts, RTS/DSR set
    mov dx, WORD PTR [bp+_port$]
    add dx, 4
    mov al, 8bh
    out dx, al

    ; Disable DLAB
    mov dx, WORD PTR [bp+_port$]
    add dx, 3
    mov al, 0h
    out dx, al

    ; Write test character
    ;mov dx, WORD PTR [bp+_port$]
    ;mov al, 031h
    ;out dx, al

    ; Tell the PIC we want interrupts
    in al, 21h
    and dl, 0efh
    out 21h, al 

    ; Enable interrupts
    mov dx, WORD PTR [bp+_port$]
    add dx, 1
    mov al, 1
    out dx, al

    RestoreRegisters
    pop bp
    ret
_init_serial_port ENDP


PUBLIC _ack_pic
_ack_pic PROC FAR
    mov ax, 20h
    out 20h, ax
    ret
_ack_pic ENDP

_TEXT ENDS

END