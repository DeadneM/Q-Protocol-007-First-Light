#pragma once
#include "NativeInventory.h"
#include <mutex>
#include <optional>

namespace qp::runtime {
struct OwnedAutoSpawn {
    Ticket ticket{};
    std::uintptr_t spawner = 0;
    std::uint64_t rid = 0;
    SpawnToken token{};
    EntityReference created{};
    std::uint64_t expiresAt = 0;
    bool observed = false;
};

// Only explicit AUTO invocations enter this ledger. The native completion
// branch must still find the exact output in the request's LIVE logical batch.
// An old capacity slot, a label or an expired ticket cannot authorize routing.
class OwnedAutoSpawns {
public:
    bool Track(const Ticket& ticket, std::uintptr_t spawner, std::uint64_t rid, const SpawnToken& token,
        std::uint64_t now) {
        if (!ticket || !spawner || !rid || !token.record || token.retired || !token.source.pointer) return false;
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& entry : entries_) if (entry.ticket == ticket) return false;
        for (auto& entry : entries_) {
            if (entry.ticket && now <= entry.expiresAt) continue;
            entry = {ticket, spawner, rid, token, {}, now + 5000, false};
            return true;
        }
        return false;
    }

    template<class Reader>
    bool Observe(Reader& read, std::uintptr_t base, std::uint32_t mask,
        std::uintptr_t source, const EntityReference& created,
        const PlayerSnapshot& current, std::uint64_t now) {
        if (!created.pointer || !created.key) return false;
        std::array<OwnedAutoSpawn, 16> candidates{};
        {
            std::unique_lock<std::mutex> lock(mutex_, std::try_to_lock);
            if (!lock) return false;
            candidates = entries_;
        }
        std::optional<OwnedAutoSpawn> selected;
        for (auto entry : candidates) {
            if (!entry.ticket.Matches(current) || !current.Fresh(now) || now > entry.expiresAt ||
                entry.token.source.pointer != source || (entry.observed && entry.created != created)) continue;
            std::uint64_t rid = 0;
            if (!LiveEntity(read, base, mask, entry.token.source) ||
                !read(source + 0x120, rid) || rid != entry.rid) continue;
            SpawnBatch live{};
            if (!read(entry.spawner + 0x38, live) || live.begin != entry.token.batch.begin ||
                live.capacity != entry.token.batch.capacity || entry.token.record < live.begin ||
                entry.token.record >= live.end) continue;
            if (ReadSpawnOutput(read, entry.spawner, entry.token) != created || entry.token.retired ||
                !entry.token.jobKnown) continue;
            SpawnBatch after{};
            if (!read(entry.spawner + 0x38, after) || after != live) continue;
            if (selected) return false; // Ambiguous owners never suppress a game operation.
            entry.created = created;
            entry.observed = true;
            selected = entry;
        }
        if (!selected) return false;
        std::unique_lock<std::mutex> lock(mutex_, std::try_to_lock);
        if (!lock) return false;
        for (auto& entry : entries_) {
            if (entry.ticket == selected->ticket && entry.spawner == selected->spawner &&
                entry.token.record == selected->token.record && entry.token.source == selected->token.source) {
                entry = *selected;
                return true;
            }
        }
        return false; // A concurrent reset retired ownership.
    }

    std::optional<OwnedAutoSpawn> Find(const Ticket& ticket, std::uint64_t now) const {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& entry : entries_)
            if (entry.ticket == ticket && entry.observed && now <= entry.expiresAt) return entry;
        return {};
    }
    void Clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        entries_ = {};
    }
private:
    mutable std::mutex mutex_;
    std::array<OwnedAutoSpawn, 16> entries_{};
};
} // namespace qp::runtime
