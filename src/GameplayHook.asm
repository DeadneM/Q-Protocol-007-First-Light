OPTION CASEMAP:NONE

EXTERN QpGameplayTick:PROC
EXTERN QpOnInputBlockTransition:PROC
EXTERN g_qpRoeNativeState:DWORD
EXTERN g_qpRoeEffectiveState:DWORD
EXTERN g_qpRoeOverrideState:DWORD
EXTERN g_qpLtkObserveTrueTarget:QWORD
EXTERN g_qpLtkObserveFalseTarget:QWORD
EXTERN g_qpInputBlockTraceResume:QWORD

PUBLIC QpGameplayHook
PUBLIC QpLtkObserveHook
PUBLIC QpInputBlockTraceHook

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
    ; Native ROE state at EXE+0x191A6D8:
    ;   DL=0 / R15b=0 -> OFF
    ;   DL=0 / R15b=1 -> LICENSE TO PUNCH
    ;   DL=1          -> LICENSE TO KILL
    ;
    ; Capture the native state first, then optionally override both result
    ; channels. This keeps the game's own ROE calculation intact underneath.

    push rax
    push rcx

    ; EAX = native state (1 OFF, 2 PUNCH, 3 LTK).
    mov eax, 1
    test dl, dl
    jnz roe_native_ltk
    test r15b, r15b
    jnz roe_native_punch
    jmp roe_native_ready

roe_native_punch:
    mov eax, 2
    jmp roe_native_ready

roe_native_ltk:
    mov eax, 3

roe_native_ready:
    mov dword ptr [g_qpRoeNativeState], eax

    ; ECX = requested override. 0 means preserve native state.
    mov ecx, dword ptr [g_qpRoeOverrideState]
    test ecx, ecx
    jz roe_effective_native

    cmp ecx, 1
    je roe_force_off
    cmp ecx, 2
    je roe_force_punch
    cmp ecx, 3
    je roe_force_ltk

    ; Invalid override: fail open to native state.
    mov ecx, eax
    jmp roe_effective_store

roe_force_off:
    xor dl, dl
    mov r15b, 0
    jmp roe_effective_store

roe_force_punch:
    xor dl, dl
    mov r15b, 1
    jmp roe_effective_store

roe_force_ltk:
    mov dl, 1
    mov r15b, 0
    jmp roe_effective_store

roe_effective_native:
    mov ecx, eax

roe_effective_store:
    mov dword ptr [g_qpRoeEffectiveState], ecx

    pop rcx
    pop rax

    ; Replay the exact bytes before the original conditional jump.
    mov rdi, qword ptr [r11+06F0h]
    and r8, qword ptr [rdi+rax*8]
    test dl, dl

    ; The original bytes ended in: je EXE+0x191A763.
    jz ltk_false_path
    jmp qword ptr [g_qpLtkObserveTrueTarget]

ltk_false_path:
    jmp qword ptr [g_qpLtkObserveFalseTarget]
QpLtkObserveHook ENDP

QpInputBlockTraceHook PROC
    ; A20N diagnostic hook at EXE+0x16D0830.
    ; Original ABI:
    ;   RCX = ZCLBlockPlayerInputAction*
    ;   DL  = requested block state (0=UNBLOCK, nonzero=BLOCK)
    ;   [RSP] = caller return address
    ;
    ; Preserve the two original arguments around the C++ trace callback.
    ; The callback is read-only and queues a compact event for the worker
    ; thread, so this hook does not alter the requested gameplay state.

    mov r10, qword ptr [rsp]

    push rcx
    push rdx
    sub rsp, 28h

    mov r8, r10
    movzx edx, byte ptr [rsp+28h]
    mov rcx, qword ptr [rsp+30h]

    call QpOnInputBlockTransition

    add rsp, 28h
    pop rdx
    pop rcx

    ; Replay the exact 16 bytes replaced at EXE+0x16D0830.
    mov qword ptr [rsp+20h], rbx
    push rdi
    sub rsp, 30h
    movzx edi, dl
    mov rbx, rcx

    jmp qword ptr [g_qpInputBlockTraceResume]
QpInputBlockTraceHook ENDP

END
