#include "../src/runtime/NativePlayer.h"
#include <array>
#include <map>
#include <cstring>
#include <cstdio>
#include <cstdlib>

using namespace qp::runtime;
struct Memory {
    std::map<std::uintptr_t, unsigned char> bytes;
    std::map<std::uintptr_t, int> reads;
    bool changeBinding = false;
    template<class T> void Put(std::uintptr_t address, const T& value) {
        const auto data = reinterpret_cast<const unsigned char*>(&value);
        for (std::size_t i = 0; i < sizeof(T); ++i) bytes[address+i] = data[i];
    }
    template<class T> bool operator()(std::uintptr_t address, T& value) {
        ++reads[address];
        if (changeBinding && address == 0x20000018 && reads[address] == 2)
            Put<std::uint64_t>(address+8, 0x200000001);
        auto data = reinterpret_cast<unsigned char*>(&value);
        for (std::size_t i = 0; i < sizeof(T); ++i) {
            const auto it = bytes.find(address+i);
            if (it == bytes.end()) return false;
            data[i] = it->second;
        }
        return true;
    }
};
constexpr std::uintptr_t base = 0x140000000;
static Memory Good() {
    Memory m;
    m.Put<std::uintptr_t>(base+0x05CF8750,0x10000000);
    m.Put<std::uint32_t>(0x10000004,1);
    m.Put<std::uint32_t>(0x10000008,1);
    m.Put<std::uintptr_t>(base+0x05D1C288,0x20000000);
    m.Put<std::uintptr_t>(0x20000000,base+0x02E86538);
    m.Put(0x20000018,CharacterBinding{{0x30000008,0x100000001},0x30000000});
    m.Put<std::uintptr_t>(0x30000000,base+0x02ECE868);
    m.Put<std::uint32_t>(0x30000200,77);
    m.Put<std::uint32_t>(base+0x067043A0,0xFFFFFFFF);
    m.Put(base+0x067210C0,std::array<std::uintptr_t,2>{0x50000000,0x50000010});
    m.Put(0x50000000,EntityReference{0x40000000,0x100000002});
    m.Put<std::uintptr_t>(0x40000000,base+0x02ECAB28);
    m.Put<std::uint32_t>(0x400001AC,77);
    m.Put<std::uint32_t>(0x400001A8,91);
    m.Put<std::uintptr_t>(base+0x06453CF8,0x60000000);
    m.Put<std::uintptr_t>(0x60000108,0x60010000);
    m.Put(0x60010040,std::array<std::uintptr_t,2>{0,0});
    m.Put<std::uintptr_t>(0x60000008,0x60030000);
    m.Put(0x60030048,std::array<std::uintptr_t,2>{0,0});
    return m;
}
static int checks = 0;
static void Require(bool value,const char* why) {
    ++checks;
    if(!value){std::fprintf(stderr,"FAIL: %s\n",why);std::exit(1);}
}
int main() {
    auto m=Good();auto p=ReadNativePlayer(m,base,0xFFFFFFFF);
    Require(p.observation.locallyControlled && p.observation.gameplayAllowed,"local native character and owner establish availability");
    Require(p.observation.identity.character==91 && p.observation.identity.player==77,"runtime and owner identities remain distinct");
    Require(p.character==0x30000000 && p.loadout==0,"no loadout required to detect the local character");
    m=Good();m.Put<std::uintptr_t>(base+0x05D1C288,0);
    Require(!ReadNativePlayer(m,base,0xFFFFFFFF).observation.locallyControlled,"no character in menu/loading");
    m=Good();m.Put<std::uint32_t>(0x10000004,2);
    Require(!ReadNativePlayer(m,base,0xFFFFFFFF).observation.locallyControlled,"expired character generation rejected");
    m=Good();m.Put<std::uint32_t>(0x10000008,2);
    Require(!ReadNativePlayer(m,base,0xFFFFFFFF).observation.locallyControlled,"expired owner generation rejected");
    m=Good();m.Put<std::uintptr_t>(0x20000000,base+0x02EDE200);
    Require(!ReadNativePlayer(m,base,0xFFFFFFFF).observation.locallyControlled,"loadout cannot masquerade as the character getter");
    m=Good();m.Put<std::uint32_t>(0x400001AC,99);
    Require(!ReadNativePlayer(m,base,0xFFFFFFFF).observation.locallyControlled,"unrelated player owner rejected");
    m=Good();m.Put(base+0x067210C0,std::array<std::uintptr_t,2>{0x50000000,0x50000020});
    m.Put(0x50000010,EntityReference{0x40000000,0x100000002});
    Require(!ReadNativePlayer(m,base,0xFFFFFFFF).observation.locallyControlled,"ambiguous owner is not guessed");
    m=Good();m.Put(0x60010040,std::array<std::uintptr_t,2>{0x60020000,0x60020004});m.Put<std::uint32_t>(0x60020000,77);
    p=ReadNativePlayer(m,base,0xFFFFFFFF);
    Require(p.observation.locallyControlled && !p.observation.gameplayAllowed && !p.observation.confirmedDead,"native sequence closes the action gate without declaring death");
    m=Good();m.Put(0x60010040,std::array<std::uintptr_t,2>{0x60020000,0x60020004});m.Put<std::uint32_t>(0x60020000,123);
    Require(ReadNativePlayer(m,base,0xFFFFFFFF).observation.gameplayAllowed,"another character's sequence does not block the local player");
    m=Good();m.Put(0x60030048,std::array<std::uintptr_t,2>{0x60040000,0x60040004});m.Put<std::uint32_t>(0x60040000,77);
    p=ReadNativePlayer(m,base,0xFFFFFFFF);
    Require(p.observation.locallyControlled && !p.observation.gameplayAllowed && p.observation.confirmedDead,"native not-alive membership closes the gate and records positive death evidence");
    m=Good();m.Put<std::uintptr_t>(0x60000108,0);
    Require(!ReadNativePlayer(m,base,0xFFFFFFFF).observation.locallyControlled,"unknown control context is not asserted playable");
    m=Good();m.changeBinding=true;
    Require(!ReadNativePlayer(m,base,0xFFFFFFFF).observation.locallyControlled,"binding changed mid-observation is rejected");
    std::printf("Native player read-contract tests PASS: %d checks. Synthetic memory only; not runtime evidence.\n",checks);
    return 0;
}
