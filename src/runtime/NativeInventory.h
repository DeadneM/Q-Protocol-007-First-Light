#pragma once
#include "NativePlayer.h"
#include <array>

namespace qp::runtime {
enum class PickupResult {
    None, PlayerChanged, MaskUnavailable, ItemChanged, InventoryUnknown, OwnerUnavailable,
    AlreadyAttached, Published, NativeFault
};

inline bool CanRetryPickup(PickupResult result) {
    switch (result) {
    case PickupResult::PlayerChanged:
    case PickupResult::MaskUnavailable:
    case PickupResult::ItemChanged:
    case PickupResult::InventoryUnknown:
    case PickupResult::OwnerUnavailable:
        return true; // These returns occur before any native mutation.
    default:
        return false;
    }
}

struct NativeRange {
    std::uintptr_t begin = 0, end = 0;
    bool operator==(const NativeRange&) const = default;
};

// Native 004EA2D0: paged handle table, 100 entries per page. No native calls.
template<class Reader>
std::uintptr_t NativeHandleSlot(Reader& read, std::uintptr_t table,
    std::uint32_t handle, std::uintptr_t stride) {
    if (!table || handle == 0xFFFFFFFF) return 0;
    NativeRange pages{}, after{};
    std::uintptr_t groups = 0, generations = 0, bitmap = 0, page = 0;
    std::uint32_t used = 0;
    std::uint8_t bit = 0, generation = 0;
    const auto index = handle & 0xFFFFu, bucket = index / 100, cell = index % 100;
    if (!read(table, pages) || !pages.begin || pages.end < pages.begin ||
        (pages.end-pages.begin)%8 || pages.end-pages.begin > 656*8 ||
        index >= (pages.end-pages.begin)/8*100 ||
        !read(table+0x18, groups) || !groups || !read(table+0x30, generations) || !generations ||
        !read(groups+bucket*32, bitmap) || !bitmap ||
        !read(groups+bucket*32+0x18, used) || !used ||
        !read(bitmap+cell/8, bit) || !(bit & (1u << (cell%8))) ||
        !read(generations+index, generation) || generation != ((handle >> 16)&255) ||
        !read(pages.begin+bucket*8, page) || !page) return 0;
    std::uintptr_t groupsAfter = 0, generationsAfter = 0, pageAfter = 0, bitmapAfter = 0;
    std::uint8_t bitAfter = 0, generationAfter = 0;
    std::uint32_t usedAfter = 0;
    if (!read(table, after) || after != pages || !read(table+0x18, groupsAfter) || groupsAfter != groups ||
        !read(table+0x30, generationsAfter) || generationsAfter != generations ||
        !read(groups+bucket*32, bitmapAfter) || bitmapAfter != bitmap ||
        !read(groups+bucket*32+0x18, usedAfter) || !usedAfter ||
        !read(bitmap+cell/8, bitAfter) || !(bitAfter & (1u << (cell%8))) ||
        !read(generations+index, generationAfter) || generationAfter != generation ||
        !read(pages.begin+bucket*8, pageAfter) || pageAfter != page) return 0;
    return page+cell*stride;
}

template<class Reader>
bool LiveEntity(Reader& read, std::uintptr_t base, std::uint32_t mask, const EntityReference& ref) {
    std::uintptr_t table = 0;
    std::uint32_t generation = 0;
    const auto index = static_cast<std::uint32_t>(ref.key);
    return mask && ref.pointer && ref.key && index < 0x1000000 &&
        read(base+0x05CF8750, table) && table && read(table+std::uintptr_t(index)*4, generation) &&
        (generation & mask) == static_cast<std::uint32_t>(ref.key >> 32);
}

struct NativeItem {
    EntityReference created{};
    std::uintptr_t interfacePointer = 0;
    std::uint32_t id = 0;
    explicit operator bool() const { return interfacePointer && id && id != 0xFFFFFFFF; }
};

// 007D6E80 interface lookup, descriptor used by 015D7D0D; item ID at IItem+4A.
// Cross-check against the ID registry (01337F40), not a cast based on a label.
template<class Reader>
NativeItem ReadNativeItem(Reader& read, std::uintptr_t base, std::uint32_t mask,
    const EntityReference& created) {
    NativeItem result{};
    std::uintptr_t metadata = 0, interfaces = 0;
    NativeRange types{}, finalTypes{};
    if (!LiveEntity(read, base, mask, created) || !read(created.pointer, metadata) || !metadata ||
        !read(metadata+0x20, interfaces) || !interfaces || !read(interfaces, types) ||
        !types.begin || types.end < types.begin || (types.end-types.begin)%16 ||
        types.end-types.begin > 256*16) return result;
    std::uintptr_t item = 0;
    for (auto at = types.begin; at < types.end; at += 16) {
        std::uintptr_t descriptor = 0;
        std::int64_t offset = 0;
        if (!read(at, descriptor) || !read(at+8, offset)) return {};
        if (descriptor != base+0x067931B8) continue;
        if (item || offset < -0x100000 || offset > 0x100000) return {};
        if (offset < 0 && created.pointer < static_cast<std::uintptr_t>(-offset)) return {};
        item = offset < 0 ? created.pointer-static_cast<std::uintptr_t>(-offset)
                          : created.pointer+static_cast<std::uintptr_t>(offset);
    }
    std::uint32_t id = 0;
    std::uintptr_t manager = 0;
    EntityReference registered{};
    if (!item || !read(item+0x4A, id) || !id || id == 0xFFFFFFFF ||
        !read(base+0x06454EC8, manager) || !manager) return {};
    const auto slot = NativeHandleSlot(read, manager+0x90, id, 16);
    if (!slot || !read(slot, registered) || registered.pointer != item || registered.key != created.key ||
        !LiveEntity(read, base, mask, registered) || !read(interfaces, finalTypes) || finalTypes != types ||
        !LiveEntity(read, base, mask, created)) return {};
    result.created = created; result.interfacePointer = item; result.id = id;
    return result;
}

enum class Attachment { Unknown, Absent, Present };

// Membership path of 015D8E60: root+38, table+1A0 keyed by runtime player ID.
template<class Reader>
Attachment ReadAttachment(Reader& read, std::uintptr_t base, std::uint32_t mask,
    std::uint32_t playerId, const NativeItem& item) {
    if (!item || !LiveEntity(read, base, mask, item.created)) return Attachment::Unknown;
    std::uintptr_t root = 0, inventory = 0;
    if (!read(base+0x06453CF8, root) || !root || !read(root+0x38, inventory) || !inventory) return Attachment::Unknown;
    const auto slot = NativeHandleSlot(read, inventory+0x1A0, playerId, 24);
    NativeRange items{}, after{};
    if (!slot || !read(slot, items) || items.end < items.begin ||
        (items.end-items.begin)%4 || items.end-items.begin > 512*4 ||
        (!items.begin && items.end)) return Attachment::Unknown;
    bool present = false;
    std::array<std::uint32_t, 512> observed{};
    std::size_t count = 0;
    for (auto at = items.begin; at < items.end; at += 4) {
        std::uint32_t id = 0;
        if (!read(at, id)) return Attachment::Unknown;
        observed[count++] = id;
        if (id == item.id) present = true;
    }
    for (std::size_t i = 0; i < count; ++i) {
        std::uint32_t id = 0;
        if (!read(items.begin+i*4, id) || id != observed[i]) return Attachment::Unknown;
    }
    std::uintptr_t finalRoot = 0, finalInventory = 0;
    if (!read(slot, after) || after != items || !read(base+0x06453CF8, finalRoot) || finalRoot != root ||
        !read(root+0x38, finalInventory) || finalInventory != inventory ||
        NativeHandleSlot(read, inventory+0x1A0, playerId, 24) != slot ||
        !LiveEntity(read, base, mask, item.created)) return Attachment::Unknown;
    return present ? Attachment::Present : Attachment::Absent;
}

struct SpawnBatch { std::uintptr_t begin = 0, end = 0, capacity = 0; bool operator==(const SpawnBatch&) const = default; };
struct SpawnRecord {
    std::uint64_t tag = 0;
    EntityReference output{}, source{};
    std::uint64_t resource = 0, flags = 0;
    bool operator==(const SpawnRecord&) const = default;
};
static_assert(sizeof(SpawnRecord) == 56);
struct SpawnToken {
    std::uintptr_t record = 0;
    SpawnBatch batch{};
    std::uint64_t tag = 0;
    EntityReference source{};
    EntityReference output{};
    bool jobKnown = false;
    bool retired = false;
};

template<class Reader>
SpawnToken CaptureSpawnToken(Reader& read, std::uintptr_t spawner, std::uintptr_t itemEntry) {
    SpawnBatch batch{}, after{};
    SpawnToken token{};
    if (!read(spawner+0x38, batch) || !batch.begin || batch.end < batch.begin || batch.capacity < batch.end ||
        (batch.end-batch.begin)%56 || batch.capacity-batch.begin > 64*56) return {};
    for (auto at = batch.begin; at < batch.end; at += 56) {
        SpawnRecord record{};
        if (!read(at, record)) return {};
        if (record.source.pointer != itemEntry) continue;
        if (token.record) return {}; // Ambiguous provenance must not become a grant.
        token = {at, batch, record.tag, record.source, record.output, record.tag != UINT64_MAX, false};
    }
    if (!read(spawner+0x38, after) || after != batch) return {};
    return token;
}

// The first qword is a native loading-job handle, initially -1. It changes
// before creation completes. Observe that transition while the record belongs
// to the live batch. A completed record can then outlive the logical vector,
// but must retain the job/source/buffer captured for this exact request.
template<class Reader>
EntityReference ReadSpawnOutput(Reader& read, std::uintptr_t spawner, SpawnToken& token) {
    SpawnBatch batch{}, after{};
    SpawnRecord record{}, stable{};
    if (!token.record || token.retired || !read(spawner+0x38, batch) || batch.begin != token.batch.begin ||
        batch.capacity != token.batch.capacity || batch.end < batch.begin || batch.end > batch.capacity ||
        (batch.end-batch.begin)%56 || token.record < batch.begin ||
        token.record > batch.capacity || batch.capacity-token.record < sizeof(record)) {
        token.retired = true;
        return {};
    }
    if (!read(token.record, record) || !read(token.record, stable) || stable != record ||
        !read(spawner+0x38, after) || after != batch) return {};
    if (record.source != token.source || (token.jobKnown && record.tag != token.tag)) {
        token.retired = true;
        return {};
    }
    if (!token.jobKnown) {
        if (token.record >= batch.end || record.tag == UINT64_MAX) return {};
        token.tag = record.tag;
        token.jobKnown = true;
    }
    return record.output;
}
} // namespace qp::runtime
