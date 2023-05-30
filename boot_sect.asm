[bits 16]
[org 0x7c00]
KERNEL_OFFSET: equ 0x9000
KERNEL_SIZE: equ 127 ;in sectors
; mov bx, REAL_MODE_MSG
; call print_string
;

mov [BOOT_DRIVE], dl ;BIOS stores the boot drive in dl, copy it to BOOT_DRIVE to remember it for later
; mov ah, 0x41
; mov bx, 0x55aa
; mov dl, 0x80
; int 0x13
; jc no_extensions
;
; mov bx, EXT_MODE_MSG
; call print_string
;
; mov si, DISK_ADDR_PACK
; mov ah, 0x42
; mov dl, [BOOT_DRIVE]
; int 0x13
;
; mov bx, EXT_MODE_MSG
;
; jmp switch_to_pm
no_extensions:
mov bp, 0x8000 
mov sp, bp

mov bx, KERNEL_OFFSET ;read the sectors to 0x0000(ES):0x9000(BX)
mov dh, KERNEL_SIZE    
mov dl, [BOOT_DRIVE]

mov cl, 0x02
call disk_load



; switch to 32 bit mode
switch_to_pm:
cli			;switch off interrupts
lgdt  [gdt_descriptor]	;load gdt descriptor
mov eax, cr0		;cr0 cannot be directly accessed apparently
or eax, 0x1		;set the first bit to switch to
mov cr0, eax		;protected (32bit) mode
jmp CODE_SEG:protected_mode ;do the far jump

jmp $

gdt_start:

    gdt_null:
	dd 0x0
	dd 0x0

    gdt_code:
	dw 0xffef   ; limit (bits 0-15)
	dw 0x0	    ; base (bits 0-15)
	db 0x0	    ; base (bits 16-23)
	db 0b10011010 ;1st flags, type flags
	db 0b11001111 ;2nd flags, limit (bits 16-19)
	db 0x0	    ; base (bits 24-31)

    gdt_data:
	dw 0xffff   ; limit (bits 0-15)
	dw 0x0	    ; base (bits 0-15)
	db 0x0	    ; base (bits 16-23)
	db 0b10010010 ;1st flags, type flags
	db 0b11001111 ;2nd flags, limit (bits 16-19)
	db 0x0	    ; base (bits 24-31)

gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1 ; size of gdt
    dd gdt_start

; In protected mode, segment registers are set to the offset
; of the segment descriptor from the start of the gdt
CODE_SEG: equ gdt_code - gdt_start
DATA_SEG: equ gdt_data - gdt_start

BOOT_DRIVE: db 0

DISK_ADDR_PACK:
    db 0x10
    db 0
    dw KERNEL_SIZE
    dw KERNEL_OFFSET
    dw 0
    dd 1
    dd 0

disk_load:
    pusha
    push dx ;to remember how many sectors are read
    mov ah, 0x02 ;BIOS read sector function
    mov al, dh
    mov ch, 0x00 ;cylinder 0
    mov dh, 0x00 ;head 0
    ; mov cl, 0x02 ;second sector

    int 0x13 ;BIOS disk service interrupt
    
    ; jc disk_error ;if carry flag is set by the BIOS, ie. if error occured jump to disk_error

    pop dx ;restore the no. of sectors requested to check whether correct no. of sectors are read
    cmp dh, al
    je disk_load_end

    ; mov bx, DISK_PARTIAL_READ_MSG
    ; call print_string
    ; mov bh, dh
    ; shr bx, 4
    ; and bx, 0xff
    ; call print_hex
    ; je disk_load_end
    jc disk_error;ignore error if it is about wrong number of sectors read TODO: proper error handling
    disk_load_end:

    popa
    ret

disk_error:
    ; mov bx, DISK_ERROR_MSG
    ; call print_string
    jmp $

; DISK_ERROR_MSG: db "Disk read error!", 0
; DISK_PARTIAL_READ_MSG: db "Disk read partially", 0
; EXT_MODE_MSG: db "Extended mode supported! ", 0

; print_whitespace:
;     pusha
;
;     mov ah, 0x0e
;     mov al, ' '
;     int 0x10
;
;     popa
;     ret

; print_hex:
;     pusha
;     ; hex number in bx
;     mov ah, 0x0e
;     mov cx, bx
;     mov dx, 4 ;to count bitshift
;
;     mov al, '0'
;     int 0x10
;     mov al, 'x'
;     int 0x10
;
;     print_hex_digit:
; 	mov cx, bx
; 	and cx, word 0xf000
; 	shr cx, 12
;
; 	add cl, byte '0'
; 	cmp cx, '9'
; 	jg char
; 	jmp end
; 	char:
; 	    add cl, byte ('a' - '9' - 1)
; 	end:
; 	    mov al, cl
; 	    int 0x10
;
; 	add dx, 4
; 	shl bx, 4
; 	cmp dx, 4 * 4
; 	jle print_hex_digit 
;
;     popa
;     ret
;
    

print_string:
    pusha

    mov ah, 0x0e

    print_loop:
	cmp [bx], byte 0x0	;if the byte is 0x0,
	je loop_end		;exit from the loop
	mov al, [bx]		;load the character into ax register lower byte
	int 0x10		;display interrupt or something
	add bx, 1		;increment the address register by one 
	jmp print_loop		;loop over

    loop_end:

    popa
    ret

;
; REAL_MODE_MSG:
;     db "Hi from 16-bit real mode! ", 0
;
[bits 32]
; PROTECTED_MODE_MSG:
;     db "Successfully switched to 32-bit protected mode! ", 0
;
;this is where we (far) jump while switchting to protected mode
;(to flush the cpu pipeline)
protected_mode:
    ;initialisation--------------------------------------------
    mov ax, DATA_SEG	;set the segment registers using the gdt
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    mov ebp, 0x90000	;apparently this is the topmost position
    mov esp, ebp

    ;------setup fpu--------moved to kernel-----------------------------------
    ; mov edx, cr0
    ; ; mov ebx, 0b1100
    ; ; not ebx
    ; ; and eax, ebx
    ; mov ax, 10
    ; and  edx, ~(0b1100)
    ; mov cr0, edx
    ; finit
    ; fnstsw ax
    ; cmp ax, 0
    ; jne $
    ; ;
    ; mov eax, cr0
    ; and ax, 0xfffb
    ; or ax, 0x02
    ; mov cr0, eax
    ; mov eax, cr4
    ; or ax, 3 << 9
    ; mov cr4, eax

    ;----------------------------------------------------------
    ;do stuff in the 32 bit land!

    ; call clear_screen
    ; mov ebx, PROTECTED_MODE_MSG
    ;
    ; call print_string_pm
    ; jmp $
    ; call print_string_pm
    ;
    ; call text_mode_newline
    ; call print_string_pm
    ; call text_mode_newlin; e
    ; call 0x9000

    call KERNEL_OFFSET

    jmp $

; [bits 32]
; VIDEO_MEMORY: equ 0xb8000
; WHITE_ON_BLACK: equ 0x0f
; TEXT_MODE_ROW_LEN: equ 80
; TEXT_MODE_COL_LEN: equ 25
;
; clear_screen:
;     pusha
;
;     mov edx, VIDEO_MEMORY
;     mov ebx, ' '
;     clear_screen_loop:
; 	mov al, bl
; 	mov ah, WHITE_ON_BLACK
;
; 	cmp edx, VIDEO_MEMORY + 2 * TEXT_MODE_ROW_LEN * TEXT_MODE_COL_LEN
; 	je clear_screen_loop_end 
;
; 	mov [edx], ax
;
; 	add edx, 2
; 	jmp clear_screen_loop
;
;     clear_screen_loop_end:
;     popa
;     ret
;
;
;
; text_mode_newline:
;     add [TEXT_MODE_ROW_OFFSET], word TEXT_MODE_ROW_LEN*2
;     mov [TEXT_MODE_COL_OFFSET], word 0
;     ret
; 
; print_string_pm:
;     pusha

;     pm_print_loop:
; 	mov al, [ebx]
; 	mov ah, WHITE_ON_BLACK
;
; 	cmp al, 0
; 	je pm_loop_end
;
; 	mov edx, VIDEO_MEMORY
; 	add dx, [TEXT_MODE_COL_OFFSET] 
; 	add dx, [TEXT_MODE_ROW_OFFSET]
; 	mov [edx], eax
;
; 	add ebx, 1
; 	add [TEXT_MODE_COL_OFFSET], word 2
;
; 	cmp [TEXT_MODE_COL_OFFSET], word TEXT_MODE_ROW_LEN*2
; 	jl pm_print_loop
; 	mov [TEXT_MODE_COL_OFFSET], word 0
; 	add [TEXT_MODE_ROW_OFFSET], word TEXT_MODE_ROW_LEN*2
;
; 	jmp pm_print_loop
;
;     pm_loop_end:
;     
;     popa
;     ret
;
; TEXT_MODE_ROW_OFFSET:
;     dw 0
; TEXT_MODE_COL_OFFSET:
;     dw 0

;FIXME: I dont think this is where the partition table is supposed to be
times (218 - ($-$$)) nop      ; Pad for disk time stamp
 
DiskTimeStamp times 8 db 0    ; Disk Time Stamp
 
bootDrive db 0                ; Our Drive Number Variable
PToff dw 0                    ; Our Partition Table Entry Offset
 
times (0x1b4 - ($-$$)) nop    ; Pad For MBR Partition Table
 
UID times 10 db 0             ; Unique Disk ID
PT1 times 16 db 0             ; First Partition Entry
PT2 times 16 db 0             ; Second Partition Entry
PT3 times 16 db 0             ; Third Partition Entry
PT4 times 16 db 0             ; Fourth Partition Entry
 
dw 0xAA55                     ; Boot Signature
; times 510-($-$$) db 0
; dw 0xaa55
