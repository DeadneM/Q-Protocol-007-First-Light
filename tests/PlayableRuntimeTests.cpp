#include "../src/runtime/GameplayMailbox.h"
#include <cstdio>
#include <cstdlib>

using namespace qp::runtime;
static int checks = 0;
static void Require(bool value, const char* why) {
    ++checks;
    if (!value) { std::fprintf(stderr, "FAIL: %s\n", why); std::exit(1); }
}

int main() {
    const PlayerObservation active{{17, 31, 0x100000044}, true, true};
    PlayableSession session;
    auto state = session.Observe({}, 100);
    Require(!state.playable && state.session == 0, "menus cannot create a player session");
    auto blocked = active; blocked.gameplayAllowed = false;
    state = session.Observe(blocked, 110);
    Require(!state.playable && !session.BeginAuto(state.session, true, 110), "character existence alone cannot start AUTO");
    state = session.Observe(active, 120);
    const auto first = state;
    Require(state.playable && state.Fresh(120), "native playable observation opens the common gate");
    Require(!state.Fresh(621) && !state.Fresh(119), "stale or reversed observations cannot authorize requests");
    Require(!session.BeginAuto(state.session, false, 120), "AUTO preference respected");
    Require(session.BeginAuto(state.session, true, 120), "AUTO starts once per character session");
    Require(!session.BeginAuto(state.session, true, 121), "no duplicate AUTO while busy");
    state = session.Observe(active, 130);
    Require(state.session == first.session && state.epoch == first.epoch, "same controlled character does not create another session");
    Require(session.WeaponsProcessed(state.session, true), "weapon result belongs to the current AUTO transaction");
    Require(session.Stage() == AutoStage::NeedAmmo, "weapons alone do not finish AUTO");
    Require(session.AmmoSubmitted(state.session), "ammo starts only after weapons");
    Require(session.AmmoProcessed(state.session, true), "ammo needs its own correlated acknowledgement");
    Require(session.Stage() == AutoStage::Processed, "AUTO processing acknowledged");
    state = session.Observe(blocked, 140);
    Require(!state.playable && state.epoch != first.epoch, "loss of control retires tickets immediately");
    state = session.Observe(active, 150);
    Require(state.session == first.session && !session.BeginAuto(state.session, true, 150), "pause/resume does not repeat processed AUTO");

    auto respawn = active; ++respawn.identity.incarnation;
    state = session.Observe(respawn, 160);
    Require(state.session != first.session && session.Stage() == AutoStage::Waiting, "new incarnation rearms AUTO even with the same numeric player ID");
    Require(!session.WeaponsProcessed(first.session, true), "late package result cannot advance the new session");

    struct Payload { int amount[6]{}; };
    GameplayMailbox<Payload, bool> mailbox;
    Payload profile{{1, 2, 3, 4, 5, 6}};
    Require(!mailbox.Submit(first, profile, 999), "stale worker snapshot cannot submit manual or AUTO commands");
    const auto oldTicket = mailbox.Submit(state, profile, 160);
    Require(static_cast<bool>(oldTicket), "current playable session submits");
    profile.amount[0] = 999;
    Require(!mailbox.Submit(state, profile, 160), "bounded mailbox does not overwrite a pending payload");
    const auto claimed = mailbox.TryTake(state);
    Require(claimed && claimed->payload.amount[0] == 1, "consumer owns an immutable payload copy");
    mailbox.Cancel();
    Require(mailbox.Busy() && !mailbox.Submit(state, profile, 160), "reset cannot reuse storage owned by an in-flight command");
    Require(!mailbox.Complete(*claimed, state, true), "cancelled in-flight completion is discarded");
    Require(!mailbox.TakeReply(oldTicket), "cancelled completion cannot be mistaken for success");
    Require(!mailbox.Busy(), "consumer releases its ownership after completion");
    const auto ticket = mailbox.Submit(state, profile, 160);
    Require(ticket && ticket.request != oldTicket.request, "request numbers are never reused by reset");
    const auto current = mailbox.TryTake(state);
    Require(current.has_value(), "new request consumed exactly once");
    Require(!mailbox.TryTake(state), "same command cannot execute twice");
    Require(mailbox.Complete(*current, state, true), "matching completion accepted");
    Require(!mailbox.Submit(state, profile, 160), "an unacknowledged result cannot be overwritten by a manual request");
    Require(!mailbox.TakeReply(oldTicket), "old ticket cannot steal new reply");
    Require(mailbox.TakeReply(ticket).value_or(false), "matching requester receives its result");
    Require(!mailbox.TakeReply(ticket), "result consumed once");

    const auto retired = mailbox.Submit(state, profile, 160);
    auto next = respawn; ++next.identity.incarnation;
    const auto replacement = session.Observe(next, 170);
    Require(!mailbox.TryTake(replacement), "mission/respawn replacement rejects already queued old requests");
    Require(!mailbox.Busy() && !mailbox.TakeReply(retired), "retired request leaves no success response");

    Require(session.BeginAuto(replacement.session, true, 170), "AUTO rearmed for new character incarnation");
    session.Observe({}, 180);
    Require(session.Stage() == AutoStage::Failed, "interrupted partial AUTO is explicit, never silently repeated");
    Require(!session.AmmoSubmitted(replacement.session), "interrupted AUTO cannot publish late ammo");
    // Regression from A34: there may be no dispatch callback for seconds while
    // a menu is open. Continuous read-only observation must retain the session.
    PlayableSession polling;
    auto observed = polling.Observe(active, 1000);
    const auto stableSession = observed.session;
    const auto stableEpoch = observed.epoch;
    Require(polling.BeginAuto(stableSession,true,1000),"AUTO starts independently of dispatch cadence");
    for(std::uint64_t now=1016;now<6000;now+=16) observed=polling.Observe(active,now);
    Require(observed.Fresh(6000) && observed.session==stableSession && observed.epoch==stableEpoch,
        "long absence of dispatch does not manufacture READY edges");
    Require(polling.Stage()==AutoStage::Weapons,"no false reset of AUTO during menu interval");
    Require(RevalidateDispatch(observed,active,6000).playable,"late dispatch revalidates the continuously observed player");
    auto foreign=active; ++foreign.identity.incarnation;
    const auto refused=RevalidateDispatch(observed,foreign,6000);
    Require(!refused.playable && refused.session==stableSession && refused.epoch==stableEpoch,
        "dispatch rejects a new native identity without publishing an out-of-order session");
    Require(!RevalidateDispatch(observed,blocked,6000).playable,"native loss of control still closes dispatch");
    Require(!RevalidateDispatch(observed,active,7000).playable,"worker failure still expires authorizations");
    GameplayMailbox<Payload,bool> delayed;
    auto earliest=polling.Snapshot();
    const auto queued=delayed.Submit(earliest,profile,5992);
    Require(static_cast<bool>(queued),"safe dispatch request is queued independently of callback cadence");
    for(std::uint64_t now=6008;now<10000;now+=16) observed=polling.Observe(active,now);
    const auto dispatched=delayed.TryTake(RevalidateDispatch(observed,active,10000));
    Require(dispatched && dispatched->ticket==queued,"same queued command survives a long menu interval without retry");
    Require(delayed.Complete(*dispatched,observed,true),"delayed request completes exactly once in the same session");
    PlayableSession lifecycle;
    auto life=lifecycle.Observe(active,100);
    const auto originalSession=life.session;
    Require(lifecycle.BeginAuto(life.session,true,100),"initial lifecycle AUTO");
    lifecycle.WeaponsProcessed(life.session,true);
    lifecycle.AmmoSubmitted(life.session);
    lifecycle.AmmoProcessed(life.session,true);
    lifecycle.Observe(blocked,110);
    lifecycle.Observe({},120);
    life=lifecycle.Observe(active,130);
    Require(life.session==originalSession && !lifecycle.BeginAuto(life.session,true,130),
        "sequence and missing reads cannot rearm a processed AUTO");
    auto dead=blocked; dead.confirmedDead=true;
    auto contradictory=active; contradictory.confirmedDead=true;
    Require(!RevalidateDispatch(life,contradictory,130).playable,
        "positive death evidence overrides inconsistent playable flags at dispatch");
    life=lifecycle.Observe(dead,140);
    Require(!life.playable && life.session==originalSession,"confirmed death closes gate without early rearm");
    lifecycle.Observe({},150);
    lifecycle.Observe(blocked,160);
    life=lifecycle.Observe(active,170);
    Require(life.session==originalSession+1 && lifecycle.BeginAuto(life.session,true,170),
        "same native identity rearms once after positively observed death");
    life=lifecycle.Observe(active,180);
    Require(life.session==originalSession+1 && !lifecycle.BeginAuto(life.session,true,180),
        "continued playable frames do not repeat respawn AUTO");
    auto replacementAfterDeath=active; ++replacementAfterDeath.identity.incarnation;
    lifecycle.Observe(dead,190);
    life=lifecycle.Observe(replacementAfterDeath,200);
    Require(life.session==originalSession+2,"replacement after death creates one session, not two");
    life=lifecycle.Observe(replacementAfterDeath,210);
    Require(life.session==originalSession+2,"death latch cleared for replacement identity");
    std::printf("Playable runtime tests PASS: %d checks. Lifecycle/mailbox fixtures only; native playability and delivery require game validation.\n", checks);
    return 0;
}
