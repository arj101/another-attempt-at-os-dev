[bits 32]

mov ebx, MSG
call print_string
call print_newline
call print_string
call print_string
call print_string
mov edx,  0
mov eax, 0

loop:
    add [MSG_NUM], byte 1
    add edx,  1
    add eax, 1

    call print_string

   
    cmp eax, byte 0x08
    jl rst2end


    add [WHITE_ON_BLACK], byte 0x01
    mov eax, 1
    cmp [WHITE_ON_BLACK], byte 0x0f
    jl rst2end
    mov [WHITE_ON_BLACK], byte 1
    rst2end:

    call delay

    jmp loop


jmp $

;-----function and data declarations------

delay:
    pusha
    mov edx, 0

    d_loop:
	add edx,1
	cmp edx, 5000000
	jl d_loop
    
    popa
    ret

VIDEO_MEMORY: equ 0xb8000
WHITE_ON_BLACK: dw 0x0f
TEXT_MODE_ROW_LEN: equ 80
TEXT_MODE_COL_LEN: equ 25

scroll_up:
    pusha
    mov edx, VIDEO_MEMORY

    su_loop:
    mov ax, [edx + 160]
    mov [edx], ax 
    add edx, 2
    cmp edx, VIDEO_MEMORY+2*((TEXT_MODE_ROW_LEN-1)*TEXT_MODE_COL_LEN)
    jle su_loop

    su_loop2:
    mov al, ' '
    mov ah, 0x0f;white on black
    mov [edx], ax
    add edx, 2
    cmp edx, VIDEO_MEMORY+2*(TEXT_MODE_ROW_LEN*TEXT_MODE_COL_LEN)
    jle su_loop2


    sub [TEXT_MODE_ROW_OFFSET], word 2*TEXT_MODE_ROW_LEN
    mov [TEXT_MODE_COL_OFFSET], word 0

    popa
    ret

clear_screen:
    pusha

    mov edx, VIDEO_MEMORY
    mov ebx, ' '
    clear_screen_loop:
	mov al, bl
	mov ah, [WHITE_ON_BLACK]

	cmp edx, VIDEO_MEMORY + 2 * TEXT_MODE_ROW_LEN * TEXT_MODE_COL_LEN
	je clear_screen_loop_end 

	mov [edx], ax

	add edx, 2
	jmp clear_screen_loop

    clear_screen_loop_end:
    popa
    ret



print_newline:
    add [TEXT_MODE_ROW_OFFSET], word TEXT_MODE_ROW_LEN*2
    mov [TEXT_MODE_COL_OFFSET], word 0
    cmp [TEXT_MODE_ROW_OFFSET], word (TEXT_MODE_COL_LEN-1)*2*TEXT_MODE_ROW_LEN
    jl pn_end
    call scroll_up
    ; mov [TEXT_MODE_ROW_OFFSET], word 2*80*24

    pn_end:
    ret

print_string:
    pusha

    ps_print_loop:  

	mov al, [ebx]

	mov ah, [WHITE_ON_BLACK]

	cmp al, 0
	je ps_loop_end

	mov edx, VIDEO_MEMORY
	add dx, [TEXT_MODE_COL_OFFSET] 
	add dx, [TEXT_MODE_ROW_OFFSET]
	mov [edx], eax

	cmp edx, VIDEO_MEMORY+TEXT_MODE_COL_LEN*2*TEXT_MODE_ROW_LEN
	jl cont
	call scroll_up

	cont:

	add ebx, 1
	add [TEXT_MODE_COL_OFFSET], word 2

	cmp [TEXT_MODE_COL_OFFSET], word TEXT_MODE_ROW_LEN*2
	jl ps_print_loop
	mov [TEXT_MODE_COL_OFFSET], word 0
	add [TEXT_MODE_ROW_OFFSET], word TEXT_MODE_ROW_LEN*2

	jmp ps_print_loop

    ps_loop_end:
    
    popa
    ret

TEXT_MODE_ROW_OFFSET:
    dw TEXT_MODE_ROW_LEN*2
TEXT_MODE_COL_OFFSET:
    dw 0

MSG_: db "Hello world from program loaded from disk!! :)  ", 0
MSG: db "     "
MSG_NUM: db 0
db 0
MSG2: db "wiggle", 0

times 256*100 dw 0xeeff
