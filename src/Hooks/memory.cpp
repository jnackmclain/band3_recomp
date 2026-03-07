#include <rex/ppc/context.h>
#include <rex/ppc/function.h>
#include <rex/ppc/memory.h>
#include <rex/logging.h>
#include <cstdint>
#include <cstring>
#include "src/config.h"
#include "src/Game/DataNode.h"
#include "src/Game/DataArray.h"

extern "C" void __imp__AddHeap(PPCContext& ctx, uint8_t* base);

extern "C" PPC_FUNC(AddHeap)
{
    uint32_t arr_addr = ctx.r5.u32;

	// look at the DataArray in r5 and determine which heap we are adding to override the size
	// TODO: this does not handle pools from mem.dta, so big_hunk is not changeable, would be nice if we could also adjust this in the future
    if (arr_addr) {
        auto* arr = reinterpret_cast<const band3::DataArray*>(PPC_RAW_ADDR(arr_addr));
        uint32_t nodes_addr = arr->mNodes;
        auto* first = reinterpret_cast<const band3::DataNode*>(PPC_RAW_ADDR(nodes_addr));
        if (first->type == band3::kDataSymbol) {
            const char* name = reinterpret_cast<const char*>(PPC_RAW_ADDR(first->value));
            auto& cfg = band3::GetConfig();
            if (cfg.main_heap_size > 0 && strcmp(name, "main") == 0) {
                REXLOG_INFO("Overriding main heap size: {:#x} -> {:#x}", ctx.r4.u32, cfg.main_heap_size);
                ctx.r4.u32 = static_cast<uint32_t>(cfg.main_heap_size);
            } else if (cfg.char_heap_size > 0 && strcmp(name, "char") == 0) {
                REXLOG_INFO("Overriding char heap size: {:#x} -> {:#x}", ctx.r4.u32, cfg.char_heap_size);
                ctx.r4.u32 = static_cast<uint32_t>(cfg.char_heap_size);
            }
        }
    }

    __imp__AddHeap(ctx, base);
}

// functions to avoid using recompiled stdlib memory stuff

static inline void ppc_memcpy_impl(uint8_t* base, uint32_t dst, uint32_t src, uint32_t n) {
    uint8_t* d = PPC_RAW_ADDR(dst);
    const uint8_t* s = PPC_RAW_ADDR(src);
    std::memmove(d, s, n);
}

// void* memcpy(void* dst, const void* src, size_t n)
extern "C" PPC_FUNC(_memcpy)
{
    uint32_t dst = ctx.r3.u32;
    uint32_t src = ctx.r4.u32;
    uint32_t n = ctx.r5.u32;

    if (n > 0) {
        ppc_memcpy_impl(base, dst, src, n);
    }
    ctx.r3.u64 = dst;
}

// void* memmove(void* dst, const void* src, size_t n)
extern "C" PPC_FUNC(_memmove)
{
    uint32_t dst = ctx.r3.u32;
    uint32_t src = ctx.r4.u32;
    uint32_t n = ctx.r5.u32;

    if (n > 0) {
        ppc_memcpy_impl(base, dst, src, n);
    }
    ctx.r3.u64 = dst;
}

// void* memset(void* dst, int c, size_t n)
extern "C" PPC_FUNC(_memset)
{
    uint32_t dst = ctx.r3.u32;
    int c = ctx.r4.s32;
    uint32_t n = ctx.r5.u32;

    if (n > 0) {
        std::memset(PPC_RAW_ADDR(dst), c, n);
    }
    ctx.r3.u64 = dst;
}
