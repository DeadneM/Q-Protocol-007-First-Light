#pragma once
#include "PlayableSession.h"

namespace qp::runtime {

// Worker-owned F4 intent. Enqueue and native processing do not change the
// selected variant. Only receipt for the bound request advances alternation.
struct WeaponSwap {
    bool nextB = true;
    std::uint64_t pendingRid = 0;
    Ticket ticket{};

    void Bind(std::uint64_t rid, const Ticket& request) {
        if (pendingRid == rid && request) ticket = request;
    }
    bool Finish(const Ticket& request, bool received) {
        if (!request || !pendingRid || request != ticket) return false;
        if (received) nextB = !nextB;
        Cancel();
        return true;
    }
    void Cancel() {
        pendingRid = 0;
        ticket = {};
    }
};

} // namespace qp::runtime
