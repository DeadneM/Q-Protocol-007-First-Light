#pragma once
#include "PlayableSession.h"
#include <cstddef>

namespace qp::runtime {
struct EntityReference {
    std::uintptr_t pointer = 0;
    std::uint64_t key = 0;
    bool operator==(const EntityReference&) const = default;
};
struct CharacterBinding {
    EntityReference reference{};
    std::uintptr_t component = 0;
    bool operator==(const CharacterBinding&) const = default;
};
static_assert(sizeof(EntityReference) == 16 && sizeof(CharacterBinding) == 24);

struct NativePlayer {
    PlayerObservation observation{};
    std::uintptr_t character = 0;
    std::uintptr_t loadout = 0; // Optional graph-discovery anchor; never a readiness condition.
    const char* reason = "local character unavailable";
};

// These are read contracts extracted from GetLocalPlayerCharacterImpl and
// GetPlayerIDFromCharacter, not a heap search or another equipment resolver.
template<class Reader>
NativePlayer ReadNativePlayer(Reader& read, std::uintptr_t base, std::uint32_t mask) {
    NativePlayer out{};
    std::uintptr_t singleton = 0, vtable = 0, table = 0;
    CharacterBinding binding{}, after{};
    auto valid = [&](const EntityReference& ref) {
        std::uint32_t cell = 0;
        const auto index = static_cast<std::uint32_t>(ref.key);
        const auto generation = static_cast<std::uint32_t>(ref.key >> 32);
        return ref.pointer && table && read(table + std::uintptr_t(index) * 4, cell) &&
            (cell & mask) == generation;
    };
    if (!read(base + 0x05CF8750, table) || !table ||
        !read(base + 0x05D1C288, singleton) || !singleton ||
        !read(singleton, vtable) || vtable != base + 0x02E86538 ||
        !read(singleton + 0x18, binding) || !binding.component || !valid(binding.reference)) return out;

    // The native singleton's +28 cache is already cast to the character type.
    // Restrict its vtable to this supported image; never invoke virtual methods.
    if (!read(binding.component, vtable) || vtable < base || vtable >= base + 0x06D32000) {
        out.reason = "local character component unreadable";
        return out;
    }
    std::uint32_t spawnerId = 0, invalidId = 0;
    if (!read(binding.component + 0x200, spawnerId) ||
        !read(base + 0x067043A0, invalidId) || spawnerId == invalidId) {
        out.reason = "local character has no player owner";
        return out;
    }
    struct Range { std::uintptr_t begin = 0, end = 0; bool operator==(const Range&) const = default; };
    Range players{}, finalPlayers{};
    if (!read(base + 0x067210C0, players) || players.end < players.begin ||
        (players.end - players.begin) % sizeof(EntityReference) ||
        players.end - players.begin > 64 * sizeof(EntityReference)) {
        out.reason = "native player registry unavailable";
        return out;
    }
    EntityReference owner{}, finalOwner{};
    std::uintptr_t ownerAddress = 0;
    std::uint32_t playerId = 0;
    for (auto at = players.begin; at < players.end; at += sizeof(EntityReference)) {
        EntityReference ref{};
        std::uint32_t id = 0, runtimeId = 0;
        if (!read(at, ref) || !valid(ref) || !read(ref.pointer, vtable) ||
            vtable != base + 0x02ECAB28 || !read(ref.pointer + 0x1AC, id) || id != spawnerId ||
            !read(ref.pointer + 0x1A8, runtimeId) || !runtimeId || runtimeId == 0xFFFFFFFF) continue;
        if (owner.pointer) { out.reason = "ambiguous local player owner"; return out; }
        owner = ref; ownerAddress = at; playerId = runtimeId;
    }
    if (!owner.pointer) { out.reason = "local player owner not registered"; return out; }

    // Same membership test as ZCLIsPlayerInSequence: root+108, vector +40/+48,
    // keyed by the character's +200 spawner ID. Refuse unavailable state.
    std::uintptr_t root = 0, sequence = 0;
    Range sequences{};
    if (!read(base + 0x06453CF8, root) || !root || !read(root + 0x108, sequence) || !sequence ||
        !read(sequence + 0x40, sequences) || sequences.end < sequences.begin ||
        (sequences.end - sequences.begin) % 4 || sequences.end - sequences.begin > 4096) {
        out.reason = "player sequence state unavailable";
        return out;
    }
    bool inSequence = false;
    for (auto at = sequences.begin; at < sequences.end; at += 4) {
        std::uint32_t id = 0;
        if (!read(at, id)) { out.reason = "player sequence state unreadable"; return out; }
        if (id == spawnerId) inSequence = true;
    }
    // Native ZCLIsHumanoidAlive also excludes IDs in root+8, vector +48/+50.
    std::uintptr_t health = 0;
    Range notAlive{};
    if (!read(root + 8, health) || !health || !read(health + 0x48, notAlive) ||
        notAlive.end < notAlive.begin || (notAlive.end - notAlive.begin) % 4 ||
        notAlive.end - notAlive.begin > 16384) {
        out.reason = "native alive state unavailable";
        return out;
    }
    bool alive = true;
    for (auto at = notAlive.begin; at < notAlive.end; at += 4) {
        std::uint32_t id = 0;
        if (!read(at, id)) { out.reason = "native alive state unreadable"; return out; }
        if (id == spawnerId) alive = false;
    }
    if (!read(singleton + 0x18, after) || after != binding || !valid(binding.reference) ||
        !read(ownerAddress, finalOwner) || finalOwner != owner || !valid(owner) ||
        !read(base + 0x067210C0, finalPlayers) || finalPlayers != players) {
        out.reason = "local player changed during observation";
        return out;
    }
    std::uintptr_t finalSingleton = 0, finalTable = 0, finalRoot = 0, finalSequence = 0;
    std::uint32_t finalSpawnerId = 0, finalRuntimeId = 0, finalOwnerId = 0;
    Range finalSequences{}, finalNotAlive{};
    std::uintptr_t finalHealth = 0;
    if (!read(base + 0x05D1C288, finalSingleton) || finalSingleton != singleton ||
        !read(base + 0x05CF8750, finalTable) || finalTable != table ||
        !read(binding.component + 0x200, finalSpawnerId) || finalSpawnerId != spawnerId ||
        !read(owner.pointer + 0x1AC, finalOwnerId) || finalOwnerId != spawnerId ||
        !read(owner.pointer + 0x1A8, finalRuntimeId) || finalRuntimeId != playerId ||
        !read(base + 0x06453CF8, finalRoot) || finalRoot != root ||
        !read(root + 0x108, finalSequence) || finalSequence != sequence ||
        !read(sequence + 0x40, finalSequences) || finalSequences != sequences ||
        !read(root + 8, finalHealth) || finalHealth != health ||
        !read(health + 0x48, finalNotAlive) || finalNotAlive != notAlive) {
        out.reason = "player control context changed during observation";
        return out;
    }
    out.character = binding.component;
    out.observation.identity = {playerId, spawnerId, binding.reference.key};
    out.observation.locallyControlled = true;
    out.observation.gameplayAllowed = alive && !inSequence;
    out.observation.confirmedDead = !alive;
    out.reason = !alive ? "local player not alive" : inSequence ? "local player in native sequence" : "local character available";
    EntityReference loadout{};
    if (read(owner.pointer + 0x188, loadout) && valid(loadout) &&
        read(loadout.pointer, vtable) && vtable == base + 0x02EDE200) out.loadout = loadout.pointer;
    return out;
}
} // namespace qp::runtime
