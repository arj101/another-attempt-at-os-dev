
mov ecx, 0 ;shift length (velocity)
mov eax, 1 ;shift direction, 1-> rght
SHIFT_LEN: equ 4
MAX_LEN: equ 40
loop:
    call delay

    mov [TEXT_MODE_COL_OFFSET], ecx
    push ebx
    mov ebx, MSG2
    call print_string
    call print_newline
    pop ebx

    ;--------velocity integration-------
    cmp eax, 1
    jne m_left
    m_right:
    add ecx, SHIFT_LEN*2
    jmp m_end
    m_left:
    sub ecx, SHIFT_LEN*2
    m_end:
    ;------/velocity integration-------

    ;---------bounce logic/------------
    cmp ecx, MAX_LEN*2
    jl overflow_cmp_end
    mov ecx, MAX_LEN*2
    mov eax, 0
    overflow_cmp_end:

    cmp ecx, SHIFT_LEN*2
    jge underflow_cmp_end
    mov eax, 1
    underflow_cmp_end:
    ;-------/bounce logic-----------

    jmp loop


