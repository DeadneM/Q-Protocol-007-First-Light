OPTION CASEMAP:NONE
EXTERN QpAutoSpawnFinalizationHook:PROC
EXTERN g_qpAutoSpawnCallTarget:QWORD
PUBLIC InvokeAutoSpawnHookSmoke
.code
InvokeAutoSpawnHookSmoke PROC FRAME
    push rdi
    .pushreg rdi
    push r12
    .pushreg r12
    sub rsp, 38h
    .allocstack 38h
    .endprolog
    mov rdi, rcx
    mov r12, rdx
    lea rax, original_call
    mov qword ptr [g_qpAutoSpawnCallTarget], rax
    jmp QpAutoSpawnFinalizationHook
original_call:
    call qword ptr [rax+1B0h]
    add rsp, 38h
    pop r12
    pop rdi
    ret
InvokeAutoSpawnHookSmoke ENDP
END
