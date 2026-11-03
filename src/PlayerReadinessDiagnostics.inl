// A31 observational tracing only. Included inside namespace qp after SafeRead/Log.
struct ReadinessTrace {
    const char* stage = "not sampled";
    std::uint64_t handle0 = 0, handle1 = 0;
    std::uintptr_t loadout = 0, vtable = 0;
    DWORD exception = 0;
};
ReadinessTrace g_readinessTrace{}; // only the existing worker accesses this state

void ReportReadiness(bool ready) {
    static std::string previousStage;
    static ULONGLONG previousTime = 0;
    const auto now = GetTickCount64();
    if (ready) { previousStage.clear(); return; }
    if (previousStage == g_readinessTrace.stage && now - previousTime < 10000) return;
    previousStage = g_readinessTrace.stage; previousTime = now;
    // Compare the independent, already mapped runtime identity chain even when
    // the native loadout resolver returns empty. Never reuse it to force READY.
    std::uintptr_t root = 0, node = 0;
    std::uint32_t playerId = 0;
    const bool rootRead = SafeRead(g_exeBase + kRuntimePlayerGlobalRva, root);
    const bool nodeRead = rootRead && root && SafeRead(root + 0x10, node);
    const bool idRead = nodeRead && node && SafeRead(node + 0x30, playerId);
    Log("[A31 READY DIAG] t=%llu stage=%s handle=%016llX:%016llX loadout=%p vtable=%p expectedVtable=%p "
        "runtime{rootRead=%d root=%p nodeRead=%d node=%p idRead=%d playerId=%u} exception=0x%08lX",
        static_cast<unsigned long long>(now), g_readinessTrace.stage,
        static_cast<unsigned long long>(g_readinessTrace.handle0),
        static_cast<unsigned long long>(g_readinessTrace.handle1),
        reinterpret_cast<void*>(g_readinessTrace.loadout), reinterpret_cast<void*>(g_readinessTrace.vtable),
        reinterpret_cast<void*>(g_exeBase + kPlayerLoadoutVtableRva),
        rootRead ? 1 : 0, reinterpret_cast<void*>(root), nodeRead ? 1 : 0,
        reinterpret_cast<void*>(node), idRead ? 1 : 0, playerId, g_readinessTrace.exception);
}
