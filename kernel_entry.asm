[bits 32]
extern _start

mov ebx, DISK_MSG
call print_string_pm

mov al, 0xff
out 0xa1, al
out 0x21, al
call _start
jmp $


VIDEO_MEMORY: equ 0xb8000
WHITE_ON_BLACK: equ 0x0f
TEXT_MODE_ROW_LEN: equ 80
TEXT_MODE_COL_LEN: equ 25

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
print_string_pm:
    pusha

    pm_print_loop:
	mov al, [ebx]
	mov ah, WHITE_ON_BLACK

	cmp al, 0
	je pm_loop_end

	mov edx, VIDEO_MEMORY
	add dx, [TEXT_MODE_COL_OFFSET] 
	add dx, [TEXT_MODE_ROW_OFFSET]
	mov [edx], eax

	add ebx, 1
	add [TEXT_MODE_COL_OFFSET], word 2

	cmp [TEXT_MODE_COL_OFFSET], word TEXT_MODE_ROW_LEN*2
	jl pm_print_loop
	mov [TEXT_MODE_COL_OFFSET], word 0
	add [TEXT_MODE_ROW_OFFSET], word TEXT_MODE_ROW_LEN*2

	jmp pm_print_loop

    pm_loop_end:
    
    popa
    ret

TEXT_MODE_ROW_OFFSET:
    dw 0
TEXT_MODE_COL_OFFSET:
    dw 0

DISK_MSG: db "Hi from disk!", 0

