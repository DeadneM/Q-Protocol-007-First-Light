#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <string>
#include <map>
#include <vector>
#include <cstdarg>

std::uintptr_t g_exeBase = 0;
constexpr std::uintptr_t kRuntimePlayerGlobalRva=0x06452BF8, kPlayerLoadoutVtableRva=0x02EDD278;
std::map<std::uintptr_t,std::vector<BYTE>> memory;
template<class T> void Put(std::uintptr_t a,const T& value) {
    memory[a]=std::vector<BYTE>(sizeof(T));memcpy(memory[a].data(),&value,sizeof(T));
}
template<class T> __declspec(noinline) bool SafeRead(std::uintptr_t a,T& value) {
    auto it=memory.find(a);if(it==memory.end()||it->second.size()!=sizeof(T))return false;
    memcpy(&value,it->second.data(),sizeof(T));return true;
}
void Log(const char*,...) {}
enum class Kind { Good, False, Null, WrongType, ChangedId, ChangedNode, Exception };
Kind kind=Kind::Good;
int calls=0;std::uint32_t observedId=0;
bool __fastcall MockNative(std::uint32_t id,std::uint64_t* out) {
    ++calls;observedId=id;
    if(kind==Kind::Exception) RaiseException(EXCEPTION_ACCESS_VIOLATION,0,0,nullptr);
    if(kind==Kind::False)return false;
    out[0]=kind==Kind::Null?0:0x30000000;out[1]=0x1234;
    if(kind==Kind::WrongType)Put<std::uintptr_t>(0x30000000,0xDEAD);
    if(kind==Kind::ChangedId)Put<std::uint32_t>(0x20000030,id+1);
    if(kind==Kind::ChangedNode)Put<std::uintptr_t>(0x10000010,0x9999);
    return true;
}
#include "../src/MissionLoadoutResolver.inl"
void Reset(Kind next) {
    kind=next;calls=0;observedId=0;memory.clear();
    Put<std::uintptr_t>(g_exeBase+kRuntimePlayerGlobalRva,0x10000000);
    Put<std::uintptr_t>(0x10000010,0x20000000);
    Put<std::uint32_t>(0x20000030,201457664);
    Put<std::uintptr_t>(0x30000000,g_exeBase+kPlayerLoadoutVtableRva);
}
int main() {
    g_exeBase=reinterpret_cast<std::uintptr_t>(&MockNative)-kMissionLoadoutResolverRva;
    int checks=0;
    auto require=[&](bool ok){++checks;if(!ok){printf("FAIL check %d\n",checks);ExitProcess(1);}};
    memory[g_exeBase+kMissionLoadoutResolverRva]=std::vector<BYTE>(std::begin(kMissionResolverPreimage),std::end(kMissionResolverPreimage));
    require(ValidateMissionResolverCode());memory[g_exeBase+kMissionLoadoutResolverRva][50]^=1;
    require(!ValidateMissionResolverCode());
    void* out=nullptr;std::uint32_t id=0;
    for(Kind k:{Kind::False,Kind::Null,Kind::WrongType,Kind::ChangedId,Kind::ChangedNode,Kind::Exception}) {
        Reset(k);require(!ResolveMissionLoadout(&out,id));require(out==nullptr && id==0);
    }
    Reset(Kind::Good);Put<std::uint32_t>(0x20000030,0);
    require(!ResolveMissionLoadout(&out,id) && calls==0);
    Reset(Kind::Good);require(ResolveMissionLoadout(&out,id));
    require(out==reinterpret_cast<void*>(0x30000000) && id==201457664 && observedId==id);
    kind=Kind::False;require(!ResolveMissionLoadout(&out,id) && out==nullptr && id==0);
    printf("Mission resolver tests PASS: %d checks. Native-getter/identity/type/code guards and no cached player reuse; fixtures only.\n",checks);
    return 0;
}
