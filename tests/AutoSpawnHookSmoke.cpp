#include "../src/runtime/NativePlayer.h"
#include <array>
#include <cstdio>
#include <cstdlib>
using qp::runtime::EntityReference;
extern "C" { std::uintptr_t g_qpAutoSpawnCallTarget=0; }
extern "C" std::uint64_t InvokeAutoSpawnHookSmoke(void*,const EntityReference*);
static std::array<std::uintptr_t,0x1B8/8> vtable{};
static std::uintptr_t object=reinterpret_cast<std::uintptr_t>(vtable.data());
static const EntityReference item{0x12345678,0x90000000A};
static bool owned=false;
static int checks=0,ordinary=0,placement=0,observer=0;
static void Require(bool result,const char* why) {
    ++checks;
    if(!result){std::fprintf(stderr,"FAIL: %s\n",why);std::exit(1);}
}
extern "C" bool QpOwnsAutoSpawnCompletion(std::uintptr_t source,std::uintptr_t reference) noexcept {
    Require(source==reinterpret_cast<std::uintptr_t>(&object),"observer receives unchanged source");
    Require(*reinterpret_cast<const EntityReference*>(reference)==item,"observer receives original prepared reference");
    ++observer;return owned;
}
static std::uint64_t Original(void* source,const EntityReference* reference) {
    Require(source==&object && *reference==item,"ordinary native arguments preserved");
    ++ordinary;return 0x1234;
}
static std::uint64_t Placement(void* source,const EntityReference* reference) {
    Require(source==&object && *reference==item,"owned native arguments preserved");
    ++placement;return 0x5678;
}
int main() {
    vtable[0x188/8]=reinterpret_cast<std::uintptr_t>(&Placement);
    vtable[0x1B0/8]=reinterpret_cast<std::uintptr_t>(&Original);
    Require(InvokeAutoSpawnHookSmoke(&object,&item)==0x1234,"unowned call returns via original native instruction");
    Require(ordinary==1 && placement==0 && observer==1,"unowned world operation called exactly once");
    owned=true;
    Require(InvokeAutoSpawnHookSmoke(&object,&item)==0x5678,"owned call selects native placement and preserves return value");
    Require(ordinary==1 && placement==1 && observer==2,"owned placement called exactly once");
    Require(object==reinterpret_cast<std::uintptr_t>(vtable.data()),"source vtable untouched");
    std::printf("AUTO hook CPU smoke PASS: %d checks; no game process involved.\n",checks);
}
