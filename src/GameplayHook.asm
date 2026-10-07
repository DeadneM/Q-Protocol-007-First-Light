OPTION CASEMAP:NONE

EXTERN QpGameplayTick:PROC
EXTERN g_qpLtkObservedPacked:DWORD
EXTERN g_qpLtkObserveTrueTarget:QWORD
EXTERN g_qpLtkObserveFalseTarget:QWORD

PUBLIC QpGameplayHook
PUBLIC QpLtkObserveHook

.code

QpGameplayHook PROC
    ; Replay the exact October-2026 final epilogue that was replaced by
    ; the 13-byte absolute jump at EXE+0x194D891.
    add rsp, 100h
    pop r15
    pop r14
    pop r13
    pop r12
    pop rdi
    pop rsi
    pop rbx

    ; Preserve the original return channels while the tiny gameplay callback runs.
    push rax
    sub rsp, 30h
    movdqu xmmword ptr [rsp+20h], xmm0

    call QpGameplayTick

    movdqu xmm0, xmmword ptr [rsp+20h]
    add rsp, 30h
    pop rax
    ret
QpGameplayHook ENDP

QpLtkObserveHook PROC
    ; At EXE+0x191A6D8, DL already contains the game's native
    ; License To Kill decision for this evaluation.
    ; Store 1 = OFF, 2 = ON without disturbing the surrounding code.
    push rax
    movzx eax, dl
    inc eax
    mov dword ptr [g_qpLtkObservedPacked], eax
    pop rax

    ; Replay the exact 13 bytes before the original conditional jump.
    mov rdi, qword ptr [r11+06F0h]
    and r8, qword ptr [rdi+rax*8]
    test dl, dl

    ; The original bytes ended in: je EXE+0x191A763.
    jz ltk_false_path
    jmp qword ptr [g_qpLtkObserveTrueTarget]

ltk_false_path:
    jmp qword ptr [g_qpLtkObserveFalseTarget]
QpLtkObserveHook ENDP

END
