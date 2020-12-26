.8086

; I'm guessing because WASM is hard to find example code for in real mode

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
    
    ; Setup for int 13h call
    ; Start with first fixed disk, we'll work from there
    push es

    xor ax, ax
    xor dx, dx
    mov ah, 08h
    mov dx, [bp+_disk_id$]

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

    pop es
    pop bp
    mov ax, 0
    ret

    error1:
    pop es
    pop bp
    mov ax, -1
    ret

    _get_disk_geometry ENDP
_TEXT ENDS

END