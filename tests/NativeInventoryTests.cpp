#include "../src/runtime/NativeInventory.h"
#include <map>
#include <cstdio>
#include <cstdlib>

using namespace qp::runtime;
struct Memory {
    std::map<std::uintptr_t, unsigned char> bytes;
    std::map<std::uintptr_t, int> reads;
    std::uintptr_t changeAddress = 0;
    template<class T> void Put(std::uintptr_t at, const T& value) {
        const auto data = reinterpret_cast<const unsigned char*>(&value);
        for (std::size_t i=0; i<sizeof(T); ++i) bytes[at+i] = data[i];
    }
    template<class T> bool operator()(std::uintptr_t at, T& value) {
        if (++reads[at] == 2 && at == changeAddress) Put<std::uint32_t>(at, 999);
        auto data = reinterpret_cast<unsigned char*>(&value);
        for (std::size_t i=0; i<sizeof(T); ++i) {
            const auto found = bytes.find(at+i);
            if (found == bytes.end()) return false;
            data[i] = found->second;
        }
        return true;
    }
};
constexpr std::uintptr_t base=0x140000000, entityManager=0x40000000, inventoryAddress=0x50000000;
constexpr std::uintptr_t itemPointer=0x30010000, attachedSlot=0x51000030, attachedBegin=0x52000000;
constexpr std::uint32_t itemId=0x9010004, playerId=0xC010002;
constexpr EntityReference output{itemPointer+8,0x100000005}, source{0x2D000180,0x100000006};
constexpr std::uintptr_t spawner=0x2D000000, recordAddress=0x60000000;

static void Table(Memory& m,std::uintptr_t table,std::uintptr_t page,std::uintptr_t auxiliary) {
    m.Put(table,NativeRange{auxiliary,auxiliary+8});
    m.Put(table+0x18,auxiliary+0x1000);
    m.Put(table+0x30,auxiliary+0x3000);
    m.Put(auxiliary,page);
    m.Put(auxiliary+0x1000,auxiliary+0x2000);
    m.Put<std::uint32_t>(auxiliary+0x1018,1);
    for (std::uintptr_t i=0;i<13;++i) m.Put<std::uint8_t>(auxiliary+0x2000+i,255);
    for (std::uintptr_t i=0;i<100;++i) m.Put<std::uint8_t>(auxiliary+0x3000+i,1);
}
static Memory Good() {
    Memory m;
    m.Put<std::uintptr_t>(base+0x05CF8750,0x10000000);
    m.Put<std::uint32_t>(0x10000014,1); m.Put<std::uint32_t>(0x10000018,1);
    m.Put<std::uintptr_t>(output.pointer,0x30020000);
    m.Put<std::uintptr_t>(0x30020020,0x30030000);
    m.Put(0x30030000,NativeRange{0x30040000,0x30040010});
    m.Put<std::uintptr_t>(0x30040000,base+0x067931B8);
    m.Put<std::int64_t>(0x30040008,-8);
    m.Put(itemPointer+0x4A,itemId);
    m.Put(base+0x06454EC8,entityManager);
    Table(m,entityManager+0x90,0x41000000,0x42000000);
    m.Put(0x41000040,EntityReference{itemPointer,output.key});
    m.Put<std::uintptr_t>(base+0x06453CF8,0x53000000);
    m.Put(0x53000038,inventoryAddress);
    Table(m,inventoryAddress+0x1A0,0x51000000,0x54000000);
    m.Put(attachedSlot,NativeRange{attachedBegin,attachedBegin+4});
    m.Put(attachedBegin,itemId);
    m.Put(spawner+0x38,SpawnBatch{recordAddress,recordAddress+56,recordAddress+56});
    m.Put(recordAddress,SpawnRecord{73,output,source,0,1});
    return m;
}
static int checks=0;
static void Require(bool result,const char* why) {
    ++checks;
    if (!result) { std::fprintf(stderr,"FAIL: %s\n",why); std::exit(1); }
}
int main() {
    auto m=Good(); auto item=ReadNativeItem(m,base,0xFFFFFFFF,output);
    Require(item && item.id==itemId && item.interfacePointer==itemPointer,"typed item matches runtime ID registry and generation");
    Require(ReadAttachment(m,base,0xFFFFFFFF,playerId,item)==Attachment::Present,"native attachment membership confirms the created item");
    m=Good();m.Put<std::uint32_t>(attachedBegin,17);
    Require(ReadAttachment(m,base,0xFFFFFFFF,playerId,item)==Attachment::Absent,"another item is not this delivery");
    m=Good();m.Put(attachedSlot,NativeRange{});
    Require(ReadAttachment(m,base,0xFFFFFFFF,playerId,item)==Attachment::Absent,"known empty inventory is distinct from unreadable state");
    m=Good();m.Put<std::uintptr_t>(base+0x06453CF8,0);
    Require(ReadAttachment(m,base,0xFFFFFFFF,playerId,item)==Attachment::Unknown,"missing scene cannot authorize pickup");
    m=Good();m.Put(attachedSlot,NativeRange{attachedBegin,attachedBegin+4096});
    Require(ReadAttachment(m,base,0xFFFFFFFF,playerId,item)==Attachment::Unknown,"oversized inventory refused");
    m=Good();m.changeAddress=attachedBegin;
    Require(ReadAttachment(m,base,0xFFFFFFFF,playerId,item)==Attachment::Unknown,"in-place content mutation invalidates membership proof");
    m=Good();m.Put<std::uint32_t>(0x10000014,2);
    Require(!ReadNativeItem(m,base,0xFFFFFFFF,output),"expired object generation refused");
    Require(ReadAttachment(m,base,0xFFFFFFFF,playerId,item)==Attachment::Unknown,"stale item cannot authorize fallback pickup");
    m=Good();m.Put(0x41000040,EntityReference{itemPointer,0x200000005});
    Require(!ReadNativeItem(m,base,0xFFFFFFFF,output),"ID registry cannot identify another incarnation as the created item");
    m=Good();m.Put<std::uintptr_t>(0x30040000,base+0x6783B30);
    Require(!ReadNativeItem(m,base,0xFFFFFFFF,output),"firearm type label alone cannot replace IItem interface contract");
    m=Good();m.Put(0x30030000,NativeRange{0x30040000,0x30040000+16*257});
    Require(!ReadNativeItem(m,base,0xFFFFFFFF,output),"unbounded interface map refused");
    m=Good();m.Put<std::int64_t>(0x30040008,-0x100001);
    Require(!ReadNativeItem(m,base,0xFFFFFFFF,output),"unsupported component offset refused");
    m=Good();m.Put<std::uint8_t>(0x42002000,0);
    Require(!ReadNativeItem(m,base,0xFFFFFFFF,output),"free native handle cell is not a live item");
    m=Good();m.Put<std::uint8_t>(0x42003004,2);
    Require(!ReadNativeItem(m,base,0xFFFFFFFF,output),"runtime handle generation mismatch refused");
    m=Good();m.Put(entityManager+0x90,NativeRange{0x42000000,0x42000001});
    Require(!ReadNativeItem(m,base,0xFFFFFFFF,output),"malformed native pages refused");
    m=Good();
    auto token=CaptureSpawnToken(m,spawner,source.pointer);
    Require(token.record==recordAddress && token.output==output,"new logical batch correlates its source entry and output");
    Require(ReadSpawnOutput(m,spawner,token)==output,"live captured record can supply output");
    m.Put(spawner+0x38,SpawnBatch{recordAddress,recordAddress,recordAddress+56});
    Require(ReadSpawnOutput(m,spawner,token)==output,"owned job result survives native logical-vector clear");
    Require(!CaptureSpawnToken(m,spawner,source.pointer).record,"empty logical batch cannot adopt historical output");
    m=Good();m.Put(recordAddress,SpawnRecord{UINT64_MAX,{},source,0,0});
    auto loading=CaptureSpawnToken(m,spawner,source.pointer);
    Require(loading.record && !loading.jobKnown,"fresh loading record begins without a job handle");
    Require(!ReadSpawnOutput(m,spawner,loading).pointer,"provisional job has no completed item");
    m.Put(recordAddress,SpawnRecord{91,{},source,0,0});
    Require(!ReadSpawnOutput(m,spawner,loading).pointer && loading.jobKnown && loading.tag==91,
        "native provisional-to-assigned job transition preserves request ownership");
    m.Put(recordAddress,SpawnRecord{91,output,source,0,1});
    m.Put(spawner+0x38,SpawnBatch{recordAddress,recordAddress,recordAddress+56});
    Require(ReadSpawnOutput(m,spawner,loading)==output,"known owned job supplies output after same-frame clear");
    m=Good();m.Put(recordAddress,SpawnRecord{UINT64_MAX,{},source,0,0});
    auto missed=CaptureSpawnToken(m,spawner,source.pointer);
    m.Put(recordAddress,SpawnRecord{92,output,source,0,1});
    m.Put(spawner+0x38,SpawnBatch{recordAddress,recordAddress,recordAddress+56});
    Require(!ReadSpawnOutput(m,spawner,missed).pointer,"unobserved assigned job cannot adopt historical capacity");
    m=Good();token=CaptureSpawnToken(m,spawner,source.pointer);
    m=Good();m.Put(recordAddress,SpawnRecord{74,output,source,0,1});
    Require(!ReadSpawnOutput(m,spawner,token).pointer,"changed batch tag invalidates attribution");
    Require(token.retired,"slot reuse permanently retires old request");
    m=Good();token=CaptureSpawnToken(m,spawner,source.pointer);
    m=Good();m.Put(recordAddress,SpawnRecord{73,output,EntityReference{source.pointer,source.key+1},0,1});
    Require(!ReadSpawnOutput(m,spawner,token).pointer,"changed source generation invalidates attribution");
    m=Good();m.Put(spawner+0x38,SpawnBatch{recordAddress,recordAddress+112,recordAddress+112});
    m.Put(recordAddress+56,SpawnRecord{73,output,source,0,1});
    Require(!CaptureSpawnToken(m,spawner,source.pointer).record,"ambiguous repeated source does not become a grant");
    m=Good();m.Put(spawner+0x38,SpawnBatch{recordAddress,recordAddress+55,recordAddress+56});
    Require(!CaptureSpawnToken(m,spawner,source.pointer).record,"malformed batch refused");
    m=Good();Require(!ReadNativeItem(m,base,0,output),"missing TLS mask fails closed");
    std::printf("Native inventory tests PASS: %d checks. Synthetic read contracts only; pickup delivery requires game validation.\n",checks);
}
