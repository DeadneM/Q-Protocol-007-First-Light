#pragma once
#include <cstdint>

namespace qp::runtime {

// Identity of the controlled character, never of its inventory/loadout asset.
struct CharacterIdentity {
    std::uint32_t character = 0;
    std::uint32_t player = 0;
    std::uint64_t incarnation = 0;

    explicit operator bool() const { return character && player && incarnation; }
    bool operator==(const CharacterIdentity&) const = default;
};

struct PlayerObservation {
    CharacterIdentity identity{};
    bool locallyControlled = false;
    bool gameplayAllowed = false;
};

struct PlayerSnapshot {
    CharacterIdentity identity{};
    std::uint64_t session = 0;
    std::uint64_t epoch = 0;
    std::uint64_t observedAt = 0;
    bool playable = false;

    bool Fresh(std::uint64_t now, std::uint64_t limit = 500) const {
        return playable && now >= observedAt && now - observedAt <= limit;
    }
};

// Dispatch may close a cached authorization, but must never publish an
// observation or manufacture another session. Only the polling producer does so.
inline PlayerSnapshot RevalidateDispatch(PlayerSnapshot published,
    const PlayerObservation& native, std::uint64_t now) {
    published.playable = published.Fresh(now) && native.locallyControlled &&
        native.gameplayAllowed && static_cast<bool>(native.identity) && native.identity == published.identity;
    return published;
}

enum class AutoStage { Waiting, Weapons, NeedAmmo, Ammo, Processed, Failed };

// Only the native gameplay observer changes availability. Loadout refreshes,
// rendering and UI never create a session or decide that a character can play.
class PlayableSession {
public:
    PlayerSnapshot Observe(const PlayerObservation& observation, std::uint64_t now) {
        const bool hasIdentity = static_cast<bool>(observation.identity);
        const bool changed = hasIdentity && observation.identity != state_.identity;
        const bool playable = hasIdentity && observation.locallyControlled && observation.gameplayAllowed;
        if (changed) {
            state_.identity = observation.identity;
            ++state_.session;
            autoStage_ = AutoStage::Waiting;
        }
        if (changed || playable != state_.playable) {
            ++state_.epoch; // Retire all pending/in-flight tickets on either edge.
            if (!changed && !playable &&
                (autoStage_ == AutoStage::Weapons || autoStage_ == AutoStage::Ammo)) {
                autoStage_ = AutoStage::Failed; // Never repeat a partly applied package blindly.
            }
        }
        state_.playable = playable;
        state_.observedAt = now;
        return state_;
    }

    PlayerSnapshot Snapshot() const { return state_; }
    AutoStage Stage() const { return autoStage_; }

    bool BeginAuto(std::uint64_t session, bool enabled, std::uint64_t now) {
        if (!enabled || session != state_.session || !state_.Fresh(now) || autoStage_ != AutoStage::Waiting)
            return false;
        autoStage_ = AutoStage::Weapons;
        return true;
    }
    bool WeaponsProcessed(std::uint64_t session, bool success) {
        if (session != state_.session || autoStage_ != AutoStage::Weapons) return false;
        autoStage_ = success ? AutoStage::NeedAmmo : AutoStage::Failed;
        return true;
    }
    bool AmmoSubmitted(std::uint64_t session) {
        if (session != state_.session || autoStage_ != AutoStage::NeedAmmo || !state_.playable) return false;
        autoStage_ = AutoStage::Ammo;
        return true;
    }
    bool AmmoProcessed(std::uint64_t session, bool success) {
        if (session != state_.session || autoStage_ != AutoStage::Ammo) return false;
        autoStage_ = success ? AutoStage::Processed : AutoStage::Failed;
        return true;
    }
    void InterruptAuto(std::uint64_t session) {
        if (session == state_.session && autoStage_ != AutoStage::Waiting && autoStage_ != AutoStage::Processed)
            autoStage_ = AutoStage::Failed;
    }

private:
    PlayerSnapshot state_{};
    AutoStage autoStage_ = AutoStage::Waiting;
};

struct Ticket {
    std::uint64_t request = 0;
    std::uint64_t session = 0;
    std::uint64_t epoch = 0;
    CharacterIdentity identity{};
    explicit operator bool() const { return request != 0; }
    bool operator==(const Ticket&) const = default;
    bool Matches(const PlayerSnapshot& state) const {
        return request && state.playable && session == state.session && epoch == state.epoch && identity == state.identity;
    }
};

} // namespace qp::runtime
