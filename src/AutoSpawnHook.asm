OPTION CASEMAP:NONE

EXTERN QpOwnsAutoSpawnCompletion:PROC
EXTERN g_qpAutoSpawnCallTarget:QWORD
PUBLIC QpAutoSpawnFinalizationHook

.code
QpAutoSpawnFinalizationHook PROC
    ; Replay the argument preparation at EXE+016B75DB. This is reached only
    ; for a source entry whose current spawn index is in its native +F0 list.
    vmovups xmm0, xmmword ptr [r12]
    mov rax, qword ptr [rdi]
    lea rdx, [rsp+20h]
    mov rcx, rdi
    vmovups xmmword ptr [rsp+20h], xmm0

    ; The original call site is 16-byte aligned and already has shadow space.
    ; Preserve all volatile input registers around the read-only ownership check.
    sub rsp, 0C0h
    mov qword ptr [rsp+20h], rax
    mov qword ptr [rsp+28h], rcx
    mov qword ptr [rsp+30h], rdx
    mov qword ptr [rsp+38h], r8
    mov qword ptr [rsp+40h], r9
    mov qword ptr [rsp+48h], r10
    mov qword ptr [rsp+50h], r11
    movups xmmword ptr [rsp+60h], xmm0
    movups xmmword ptr [rsp+70h], xmm1
    movups xmmword ptr [rsp+80h], xmm2
    movups xmmword ptr [rsp+90h], xmm3
    movups xmmword ptr [rsp+0A0h], xmm4
    movups xmmword ptr [rsp+0B0h], xmm5
    call QpOwnsAutoSpawnCompletion
    mov byte ptr [rsp+58h], al
    mov rax, qword ptr [rsp+20h]
    mov rcx, qword ptr [rsp+28h]
    mov rdx, qword ptr [rsp+30h]
    mov r8, qword ptr [rsp+38h]
    mov r9, qword ptr [rsp+40h]
    mov r10, qword ptr [rsp+48h]
    mov r11, qword ptr [rsp+50h]
    movups xmm0, xmmword ptr [rsp+60h]
    movups xmm1, xmmword ptr [rsp+70h]
    movups xmm2, xmmword ptr [rsp+80h]
    movups xmm3, xmmword ptr [rsp+90h]
    movups xmm4, xmmword ptr [rsp+0A0h]
    movups xmm5, xmmword ptr [rsp+0B0h]
    cmp byte ptr [rsp+58h], 0
    je original_world_throw
    add rsp, 0C0h
    ; Same native placement completion as the ordinary non-throw branch.
    ; The game's original [rax+1B0] call reads the +188 method instead.
    ; Return to the original CALL instruction, preserving native unwind/return.
    ; No object/vtable is written and no pickup is published here.
    lea rax, [rax-28h]
    jmp qword ptr [g_qpAutoSpawnCallTarget]
original_world_throw:
    add rsp, 0C0h
    jmp qword ptr [g_qpAutoSpawnCallTarget]
QpAutoSpawnFinalizationHook ENDP
END
