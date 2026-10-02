OPTION CASEMAP:NONE

EXTERN QpGameplayTick:PROC
PUBLIC QpGameplayHook

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

END
