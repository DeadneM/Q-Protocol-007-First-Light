#include "../src/runtime/OwnedAutoSpawn.h"
#include <map>
#include <cstdio>
#include <cstdlib>
using namespace qp::runtime;
struct Memory {
    std::map<std::uintptr_t, unsigned char> bytes;
    template<class T> void Put(std::uintptr_t at, const T& value) {
        const auto data = reinterpret_cast<const unsigned char*>(&value);
        for (std::size_t i=0;i<sizeof(T);++i) bytes[at+i]=data[i];
    }
    template<class T> bool operator()(std::uintptr_t at,T& value) {
        auto data=reinterpret_cast<unsigned char*>(&value);
        for (std::size_t i=0;i<sizeof(T);++i) {
            const auto found=bytes.find(at+i);
            if (found==bytes.end()) return false;
            data[i]=found->second;
        }
        return true;
    }
};
constexpr std::uintptr_t base=0x140000000, spawner=0x30000000, recordAddress=0x40000000;
constexpr std::uint64_t fixtureRid=0x12345678;
constexpr EntityReference source{0x30001000,0x100000006}, output{0x50000000,0x100000007};
const Ticket first{1,1,1,{1,2,3}}, second{2,1,1,{1,2,3}};
static Memory Good() {
    Memory m;
    m.Put<std::uintptr_t>(base+0x05CF8750,0x10000000);
    m.Put<std::uint32_t>(0x10000018,1);
    m.Put(source.pointer+0x120,fixtureRid);
    m.Put(spawner+0x38,SpawnBatch{recordAddress,recordAddress+56,recordAddress+56});
    m.Put(recordAddress,SpawnRecord{UINT64_MAX,{},source,0,0});
    return m;
}
static void Complete(Memory& m,std::uint64_t job=99) {
    m.Put(recordAddress,SpawnRecord{job,output,source,0,1});
}
static int checks=0;
static void Require(bool result,const char* why) {
    ++checks;
    if (!result) {std::fprintf(stderr,"FAIL: %s\n",why);std::exit(1);}
}
int main() {
    const PlayerSnapshot current{{1,2,3},1,1,100,true};
    OwnedAutoSpawns ledger;
    auto m=Good();auto token=CaptureSpawnToken(m,spawner,source.pointer);
    Complete(m);
    Require(!ledger.Observe(m,base,~0u,source.pointer,output,current,110),"ordinary native creation has no AUTO ownership");
    Require(ledger.Track(first,spawner,fixtureRid,token,100),"explicit AUTO invocation captured");
    Require(!ledger.Find(first,110),"invocation alone is not a completion");
    Require(!ledger.Observe(m,base,~0u,source.pointer,{},current,110),"missing created object cannot authorize routing");
    Require(!ledger.Track(first,spawner,fixtureRid,token,110),"duplicate tracking cannot overwrite proof");
    Require(ledger.Observe(m,base,~0u,source.pointer,output,current,120),"native live completion learns previously provisional job");
    const auto observation=ledger.Find(first,130);
    Require(observation && observation->created==output && observation->token.tag==99 && observation->token.jobKnown,
        "worker receives exact observed output and assigned native job");
    m.Put(spawner+0x38,SpawnBatch{recordAddress,recordAddress,recordAddress+56});
    Require(ledger.Find(first,140).has_value(),"native acknowledgement survives same-frame logical clear");
    Require(!ledger.Observe(m,base,~0u,source.pointer,output,current,140),"historical capacity cannot authorize a native operation");
    Require(!ledger.Find(second,140),"another request cannot consume this acknowledgement");
    ledger.Clear();Require(!ledger.Find(first,140),"reset retires old completion");
    m=Good();token=CaptureSpawnToken(m,spawner,source.pointer);ledger.Track(first,spawner,fixtureRid,token,100);Complete(m);
    auto changed=current;++changed.epoch;
    Require(!ledger.Observe(m,base,~0u,source.pointer,output,changed,120),"old epoch leaves game operation unchanged");
    changed=current;++changed.session;
    Require(!ledger.Observe(m,base,~0u,source.pointer,output,changed,120),"old session leaves game operation unchanged");
    changed=current;++changed.identity.incarnation;
    Require(!ledger.Observe(m,base,~0u,source.pointer,output,changed,120),"replacement player leaves game operation unchanged");
    Require(!ledger.Observe(m,base,~0u,source.pointer,output,current,700),"stale player observation rejected");
    changed=current;changed.observedAt=5200;
    Require(!ledger.Observe(m,base,~0u,source.pointer,output,changed,5200),"expired ownership rejected even for fresh player");
    m.Put(source.pointer+0x120,fixtureRid+1);
    Require(!ledger.Observe(m,base,~0u,source.pointer,output,current,120),"changed source resource rejected");
    m.Put(source.pointer+0x120,fixtureRid);m.Put<std::uint32_t>(0x10000018,2);
    Require(!ledger.Observe(m,base,~0u,source.pointer,output,current,120),"reused source generation rejected");
    m.Put<std::uint32_t>(0x10000018,1);
    Require(!ledger.Observe(m,base,~0u,source.pointer,EntityReference{output.pointer,output.key+1},current,120),"wrong created incarnation rejected");
    ledger.Clear();m=Good();Complete(m);token=CaptureSpawnToken(m,spawner,source.pointer);
    ledger.Track(first,spawner,fixtureRid,token,100);Complete(m,100);
    Require(!ledger.Observe(m,base,~0u,source.pointer,output,current,120),"reused job cannot belong to earlier ticket");
    ledger.Clear();m=Good();token=CaptureSpawnToken(m,spawner,source.pointer);
    ledger.Track(first,spawner,fixtureRid,token,100);ledger.Track(second,spawner,fixtureRid,token,100);Complete(m);
    Require(!ledger.Observe(m,base,~0u,source.pointer,output,current,120),"ambiguous overlapping requests do not suppress a game operation");
    std::printf("Owned AUTO completion tests PASS: %d checks; native routing still requires game validation.\n",checks);
}
