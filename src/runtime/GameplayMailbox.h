#pragma once
#include "PlayableSession.h"
#include <mutex>
#include <optional>

namespace qp::runtime {

// The payload is copied under one lock; reset never reopens storage being copied.
// Native operations run outside this lock. A late result cannot match a new ticket.
template<class Payload, class Result>
class GameplayMailbox {
public:
    struct Command { Ticket ticket{}; Payload payload{}; };
    struct Reply { Ticket ticket{}; Result result{}; };

    Ticket Submit(const PlayerSnapshot& state, const Payload& payload, std::uint64_t now) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!state.Fresh(now) || pending_ || inFlight_ || reply_) return {};
        const Ticket ticket{++nextRequest_, state.session, state.epoch, state.identity};
        pending_ = Command{ticket, payload};
        latest_ = ticket;
        reply_.reset();
        return ticket;
    }
    std::optional<Command> TryTake(const PlayerSnapshot& current) {
        std::unique_lock<std::mutex> lock(mutex_, std::try_to_lock);
        if (!lock || !pending_) return {};
        const Command command = *pending_;
        pending_.reset();
        if (!command.ticket.Matches(current)) {
            if (latest_ == command.ticket) latest_ = {};
            return {};
        }
        inFlight_ = command.ticket;
        return command;
    }
    bool Complete(const Command& command, const PlayerSnapshot& current, const Result& result) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (inFlight_ != command.ticket) return false;
        inFlight_ = {};
        if (latest_ != command.ticket || !command.ticket.Matches(current)) return false;
        reply_ = Reply{command.ticket, result};
        return true;
    }
    std::optional<Result> TakeReply(const Ticket& ticket) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!reply_ || reply_->ticket != ticket) return {};
        const Result result = reply_->result;
        reply_.reset();
        return result;
    }
    void Cancel() {
        std::lock_guard<std::mutex> lock(mutex_);
        pending_.reset();
        reply_.reset();
        latest_ = {};
        // An already claimed operation keeps exclusive ownership until Complete.
        // Its eventual reply is rejected because latest_ was retired.
    }
    bool Busy() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return pending_.has_value() || static_cast<bool>(inFlight_);
    }

private:
    mutable std::mutex mutex_;
    std::uint64_t nextRequest_ = 0;
    Ticket latest_{};
    Ticket inFlight_{};
    std::optional<Command> pending_;
    std::optional<Reply> reply_;
};

} // namespace qp::runtime
