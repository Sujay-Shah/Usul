#pragma once
// =============================================================
//  rhi_pool.hpp — Generational pool allocator
//
//  Manages fixed-capacity arrays of typed "slots".  Each slot is
//  identified by a Handle<T> that encodes both the array index
//  and a generation counter.  When a slot is freed its generation
//  advances; any handle still pointing at the old generation is
//  immediately detected as stale.
//
//  Properties
//  ----------
//  • O(1)  alloc / free / get  — no trees, no hash maps
//  • Zero heap allocation      — data lives inside the Pool object
//  • Cache-friendly            — flat array, low indices first
//  • Stale-handle detection    — generation check on every get()
//  • Double-free detection     — alive flag checked on every free()
//  • Leak detection            — debug reset() reports live slots
//  • for_each()                — iterate live slots for shutdown
//
//  Overrideable hooks (define before including this header)
//  --------------------------------------------------------
//  RHI_POOL_LOG(fmt, ...)  — default: fprintf(stderr, ...)
//  RHI_POOL_ASSERT(c,msg)  — default: assert(c)
// =============================================================

#include "rhi_types.hpp"
#include <cstring>
#include <cassert>
#include <new>
#include <type_traits>

#ifndef RHI_POOL_LOG
  #include <cstdio>
  #define RHI_POOL_LOG(fmt, ...) fprintf(stderr, "[rhi::Pool] " fmt, ##__VA_ARGS__)
#endif

#ifndef RHI_POOL_ASSERT
  #define RHI_POOL_ASSERT(cond, msg) assert((cond) && (msg))
#endif

namespace rhi {

// =============================================================
//  SlotHeader
//  Every backend slot struct must embed this as its FIRST member
//  (checked by static_assert in Pool).
// =============================================================
struct alignas(4) SlotHeader
{
    u16  gen   = 0;     // generation counter; wraps, never 0
    bool alive = false;
    u8   _pad  = 0;
};
static_assert(sizeof(SlotHeader) == 4);

// =============================================================
//  handle_make  — pack (index, gen) into a handle id
// =============================================================
[[nodiscard]] inline constexpr u32 handle_make(u32 index, u32 gen) noexcept
{
    return ((gen & HANDLE_GEN_MASK) << HANDLE_INDEX_BITS)
         | (index & HANDLE_INDEX_MASK);
}

// =============================================================
//  Pool<SlotT, Capacity, HandleT>
//
//  SlotT    — your slot struct; must be standard-layout with
//             SlotHeader hdr as its first member
//  Capacity — total slots INCLUDING the reserved slot-0 sentinel;
//             usable capacity = Capacity - 1
//  HandleT  — typed handle (e.g. rhi::Buffer)
//
//  IMPORTANT: for large Capacity values (like MAX_BUFFERS = 1M)
//  declare the Pool as a file-scope static or global — never
//  on the stack.
// =============================================================

template<typename SlotT, u32 Capacity, typename HandleT>
class Pool
{
    static_assert(std::is_standard_layout_v<SlotT>,
        "Slot must be standard-layout so SlotHeader is at a stable offset.");
    static_assert(offsetof(SlotT, hdr) == 0,
        "SlotHeader 'hdr' must be the very first member of the slot struct.");
    static_assert(Capacity > 1,
        "Capacity must be > 1 (slot 0 is the null sentinel).");
    static_assert(Capacity <= (1u << HANDLE_INDEX_BITS),
        "Capacity exceeds handle index range.");

public:
    Pool() noexcept  { _init_freelist(); }
    ~Pool() noexcept = default;

    Pool(const Pool&)            = delete;
    Pool& operator=(const Pool&) = delete;

    // ----------------------------------------------------------
    //  alloc() — pop a free slot, stamp it alive, return handle.
    //  Returns null handle if pool is exhausted.
    // ----------------------------------------------------------
    [[nodiscard]] HandleT alloc() noexcept
    {
        if (m_top == 0)
        {
            RHI_POOL_LOG("exhausted (cap=%u used=%u)\n", Capacity, used());
            RHI_POOL_ASSERT(false, "Pool exhausted — raise its capacity");
            return HandleT{};
        }

        const u32    index = m_freelist[--m_top];
        SlotHeader  *hdr   = _hdr(index);

        // Placement-new resets non-trivial members; re-stamp hdr after
        if constexpr (!std::is_trivially_constructible_v<SlotT>)
        {
            const u16 saved_gen = hdr->gen ? hdr->gen : 1;
            new (&m_slots[index]) SlotT{};
            _hdr(index)->gen   = saved_gen;
        }

        hdr = _hdr(index);
        if (hdr->gen == 0) hdr->gen = 1; // skip null-gen on first use
        hdr->alive = true;
        ++m_total_allocs;

        return HandleT{ handle_make(index, hdr->gen) };
    }

    // ----------------------------------------------------------
    //  free() — validate handle, bump generation, return slot.
    //  Returns false on null / stale / double-free handles.
    // ----------------------------------------------------------
    bool free(HandleT h) noexcept
    {
        if (!h) return false;

        const u32 idx = h.index();
        const u32 gen = h.gen();

        if (idx == 0 || idx >= Capacity)
        {
            RHI_POOL_LOG("free: index %u out of range\n", idx);
            RHI_POOL_ASSERT(false, "Pool::free — index out of range");
            return false;
        }

        SlotHeader *hdr = _hdr(idx);

        if (!hdr->alive)
        {
            RHI_POOL_LOG("free: double-free at index %u\n", idx);
            RHI_POOL_ASSERT(false, "Pool::free — double-free detected");
            return false;
        }

        if (hdr->gen != static_cast<u16>(gen))
        {
            RHI_POOL_LOG("free: stale handle (slot gen=%u handle gen=%u idx=%u)\n",
                         hdr->gen, gen, idx);
            RHI_POOL_ASSERT(false, "Pool::free — stale handle");
            return false;
        }

        if constexpr (!std::is_trivially_destructible_v<SlotT>)
            m_slots[idx].~SlotT();

        hdr->alive = false;
        // Advance generation, wrapping around but skipping 0
        const u32 next = ((static_cast<u32>(hdr->gen) + 1u) & HANDLE_GEN_MASK);
        hdr->gen = static_cast<u16>(next == 0 ? 1u : next);

        m_freelist[m_top++] = idx;
        return true;
    }

    // ----------------------------------------------------------
    //  get() — O(1) validated lookup; returns nullptr on failure.
    //  Null / stale / dead handles all return nullptr.
    // ----------------------------------------------------------
    [[nodiscard]] SlotT* get(HandleT h) noexcept
    {
        const u32 id = h.id;
        if (id == 0) [[unlikely]] return nullptr;

        const u32 idx = id & HANDLE_INDEX_MASK;
        const u32 gen = (id >> HANDLE_INDEX_BITS) & HANDLE_GEN_MASK;

        if (idx == 0 || idx >= Capacity) [[unlikely]] return nullptr;

        const SlotHeader *hdr = _hdr(idx);
        if (!hdr->alive || hdr->gen != static_cast<u16>(gen)) [[unlikely]]
            return nullptr;

        return &m_slots[idx];
    }

    [[nodiscard]] const SlotT* get(HandleT h) const noexcept
    {
        return const_cast<Pool*>(this)->get(h);
    }

    // get_checked — asserts in debug, undefined behaviour in release
    // Use for internal backend paths where a bad handle is a programmer error.
    [[nodiscard]] SlotT& get_checked(HandleT h) noexcept
    {
        SlotT *s = get(h);
        RHI_POOL_ASSERT(s != nullptr, "Pool::get_checked — invalid or stale handle");
        return *s;
    }
    [[nodiscard]] const SlotT& get_checked(HandleT h) const noexcept
    {
        const SlotT *s = get(h);
        RHI_POOL_ASSERT(s != nullptr, "Pool::get_checked — invalid or stale handle");
        return *s;
    }

    // ----------------------------------------------------------
    //  for_each() — iterate every live slot.
    //  Usage: pool.for_each([](SlotT& s){ vkDestroyBuffer(...); });
    //  Not safe to alloc/free inside fn.
    // ----------------------------------------------------------
    template<typename Fn>
    void for_each(Fn&& fn)
    {
        for (u32 i = 1; i < Capacity; ++i)
            if (m_slots[i].hdr.alive) fn(m_slots[i]);
    }

    template<typename Fn>
    void for_each(Fn&& fn) const
    {
        for (u32 i = 1; i < Capacity; ++i)
            if (m_slots[i].hdr.alive) fn(m_slots[i]);
    }

    // ----------------------------------------------------------
    //  Stats
    // ----------------------------------------------------------
    [[nodiscard]] u32 used()         const noexcept { return (Capacity - 1u) - m_top; }
    [[nodiscard]] u32 free_slots()   const noexcept { return m_top; }
    [[nodiscard]] u32 capacity()     const noexcept { return Capacity - 1u; }
    [[nodiscard]] u64 total_allocs() const noexcept { return m_total_allocs; }

    // ----------------------------------------------------------
    //  reset() — wipe state.
    //  GPU objects MUST be destroyed before calling this.
    //  In debug, warns about leaked live slots.
    // ----------------------------------------------------------
    void reset() noexcept
    {
#ifndef NDEBUG
        u32 leaked = 0;
        for (u32 i = 1; i < Capacity; ++i)
            if (m_slots[i].hdr.alive) ++leaked;
        if (leaked)
            RHI_POOL_LOG("reset: %u slot(s) still alive — GPU resource leak!\n", leaked);
#endif
        // Run destructors for non-trivial types
        if constexpr (!std::is_trivially_destructible_v<SlotT>)
            for (u32 i = 1; i < Capacity; ++i)
                if (m_slots[i].hdr.alive) m_slots[i].~SlotT();

        // Zero entire array preserving gen counters — zero hdr is fine;
        // first alloc after reset bumps gen 0 → 1.
        std::memset(m_slots, 0, sizeof(m_slots));
        m_total_allocs = 0;
        _init_freelist();
    }

private:
    [[nodiscard]] SlotHeader* _hdr(u32 i) noexcept
    {
        return reinterpret_cast<SlotHeader*>(&m_slots[i]);
    }
    [[nodiscard]] const SlotHeader* _hdr(u32 i) const noexcept
    {
        return reinterpret_cast<const SlotHeader*>(&m_slots[i]);
    }

    void _init_freelist() noexcept
    {
        m_top = 0;
        // Push in reverse so index 1 comes out first (low indices = hot cache)
        for (u32 i = Capacity; i > 1; --i)
            m_freelist[m_top++] = i - 1u;
    }

    // Data — 64-byte aligned so slot[0] starts on a cache-line boundary
    alignas(64) SlotT m_slots   [Capacity];
                u32   m_freelist[Capacity];
                u32   m_top          = 0;
                u64   m_total_allocs = 0;
};

} // namespace rhi
