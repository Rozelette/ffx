// Heap allocator ("SgMem" in the game's own error strings).
//
// Every allocation starts with a 0x10 byte MemControlBlock header. Headers form a doubly linked list in
// address order, and a block runs up to the next header. Each heap starts with a free block and ends with
// a MEM_OWNER_END sentinel. Free blocks are not merged when freed. Freeing sets heap->hasFreedNodes, and
// the next allocation merges neighbours with Mem_MergeFreeBlocks.
//
// Heap 0 is the main heap. Heap 1 is a debug heap that only exists when gDebugMode (debug mode) is set.
// sHeaps has room for three heaps, but Mem_Init only sets up two.
//
// The public wrappers (Mem_Alloc, Mem_Free, malloc, ...) are in mem2.c.

#include "common.h"
#include "mem.h"

// Debug allocation name table, only present when gDebugMode is set
#define MEM_DEBUG_INFO ((MemDebugInfo*)0x05FB0000)
#define MEM_DEBUG_INFO_COUNT 0x800

// MemControlBlock.info bits holding the block's MEM_DEBUG_INFO index
#define MCB_DEBUG_INDEX_MASK 0x07FF0000

#define BLOCK_SIZE(block) (((uintptr_t)block->next - (uintptr_t)block) - sizeof(MemControlBlock))

#define PTR_TO_BLOCK(ptr) ((MemControlBlock*)ptr - 1)
#define BLOCK_TO_PTR(block) ((void*)(block + 1))

extern s8 gDebugMode;

// When set, the next failed allocation returns NULL instead of reporting the failure and retrying.
// Cleared by every allocation.
static s32 sDebugFailedAlloc = 0;

// Four character names for each owner ID, indexed by MemControlBlock.attr.bits.owner. 0 is "----" for
// free blocks. 14 (MEM_OWNER_ANY) and 15 (MEM_OWNER_END) are blank. The rest look like developer name
// abbreviations. Nothing in the retail build reads this table, so it was probably used by the stripped
// printing in Mem_PrintBlock.
static char* sMemOwnerNames[] = {
    "----", "aka ", "ega ", "toni", "yoni", "kata", "masa", "sugi",
    "nari", "hase", "thas", "sasa", "yana", "ishi", "    ", "    ",
};

static const char sEmptyDebugName[] = "";

static s32 sMemCurAlign; // Alignment mask for the next forward allocation, see Mem_SetAlignment
static Heap sHeaps[3];
static intptr_t sMemSavedRa;            // Packed return address stored into the next block's attr.bits.ra
static s8 sMemUnk54;                    // Set but never read
static s8 sMemUnk55;                    // Set but never read
static MemControlBlock* sCompactCursor; // Where Mem_CompactBlock starts looking for free space

void Mem_InsertFreeBlock(s32, MemControlBlock*, MemControlBlock*, MemControlBlock*);
void Mem_SaveRA(void*);
void Mem_PrintBlock(MemControlBlock*, s32);
void Mem_DumpHeap(Heap*);

void Mem_SetCompactCursor(void*);

// libc
void* memcpy(void* dst, const void* src, unsigned int size);
void* memmove(void* dst, const void* src, unsigned int size);

extern void Cache_Dump(char*);

void Mem_ClearDbgArea(void) {
    s32 next;
    MemDebugInfo* curInfo;

    if (gDebugMode != 0) {
        next = MEM_DEBUG_INFO_COUNT - 1;
        curInfo = &MEM_DEBUG_INFO[MEM_DEBUG_INFO_COUNT - 1];
        do {
            curInfo->name[0] = '\0';
            next = next + -1;
            curInfo--;
        } while (-1 < next);
    }
}

// Releases the block's MEM_DEBUG_INFO slot
void Mem_ClearDebugName(void* ptr) {
    MemControlBlock* mcb = PTR_TO_BLOCK(ptr);

    if ((gDebugMode != 0) && (ptr != NULL)) {
        if (mcb->info.raw & MCB_DEBUG_INDEX_MASK) {
            MEM_DEBUG_INFO[(mcb->info.raw >> 16) & 0x7FF].name[0] = '\0';
            mcb->info.raw = (u32)(mcb->info.raw & ~MCB_DEBUG_INDEX_MASK);
        }
    }
}

// Returns the block's "file:line" name, or an empty string if it has none
MemDebugInfo* Mem_GetDebugName(void* ptr) {
    MemControlBlock* mcb = PTR_TO_BLOCK(ptr);
    MemDebugInfo* arr = MEM_DEBUG_INFO;

    if (gDebugMode == 0) {
        return (MemDebugInfo*)&sEmptyDebugName;
    }

    if (ptr == NULL) {
        return (MemDebugInfo*)&sEmptyDebugName;
    }

    if ((mcb->info.raw & MCB_DEBUG_INDEX_MASK) == 0) {
        return (MemDebugInfo*)&sEmptyDebugName;
    }
    return &arr[(mcb->info.raw >> 16) & 0x7FF];
}

// libc
int sprintf(char* str, const char* fmt, ...);
void* memset(void*, s32, s32);
int strlen(char*);
int strncpy(char*, char*, int);
int printf(const char* fmt, ...);

// Stores "file:line" in a MEM_DEBUG_INFO slot for the block. File names that are too long keep their end
// and get a '*' as the first character. Reuses the block's slot if it already has one, otherwise takes
// the first empty slot (slot 0 is never used).
void Mem_SetDebugName(void* ptr, char* file, int line) {
    MemControlBlock* mcb = PTR_TO_BLOCK(ptr);
    MemDebugInfo* info = MEM_DEBUG_INFO;
    int index = 0;
    char name[0x30];
    s32 i;

    if (gDebugMode == 0 || ptr == NULL) {
        return;
    }

    if (file != NULL) {
        char lineStr[0x10];
        i = ' ';
        if (line > 0) {
            i = ':';
        }
        if (line > 0) {
            sprintf(lineStr, "%d", line);
        } else {
            lineStr[0] = '\0';
        }

        if (file != NULL) {
            int skip = strlen(file) + strlen(lineStr) - 30;
            if (skip < 0) {
                skip = 0;
            }
            sprintf(name, "%s%c%s", file + skip, i, lineStr);
            if (skip != 0) {
                name[0] = '*';
            }
        } else {
            sprintf(name, "%s", lineStr);
        }

        if (mcb->info.raw & MCB_DEBUG_INDEX_MASK) {
            memset((void*)(((mcb->info.raw >> 16) & 0x7FF) * sizeof(MemDebugInfo) + (u32)info), 0,
                   sizeof(MemDebugInfo));
            strncpy(info[mcb->info.bits.debugIndex].name, name, sizeof(info->name) - 1);
            info[mcb->info.bits.debugIndex].name[sizeof(info->name) - 1] = '\0';
            index = (mcb->info.raw >> 16) & 0x7FF;
        } else {
            i = 1;
            do {
                if (info[i].name[0] == '\0') {
                    memset(&info[i], 0, sizeof(MemDebugInfo));
                    index = i;
                    strncpy(info[i].name, name, sizeof(info->name) - 1);
                    info[i].name[sizeof(info->name) - 1] = '\0';
                    break;
                }
                i++;
            } while (i < MEM_DEBUG_INFO_COUNT);
        }
    }

    mcb->info.bits.debugIndex = index;
}

// Records the caller of the caller as the block's allocation site
void Mem_SetBlockRA(void* ptr) {
    MemControlBlock* mcb = PTR_TO_BLOCK(ptr);
    if (ptr != 0) {
        void* ra;
        GetRa(&ra);
        Mem_SaveRA(ra);
        mcb->attr.bits.ra = sMemSavedRa;
    }
}

void Mem_SetUnk54(s8 arg0) {
    sMemUnk54 = arg0;
}

void Mem_SetUnk55(s8 arg0) {
    sMemUnk55 = arg0;
}

// Sets up a heap as one free block followed by the end sentinel
void Mem_InitHeap(u32 heapIdx, intptr_t heapStart, intptr_t heapEnd) {
    Heap* heap;
    MemControlBlock* end;
    MemControlBlock* end_3;
    MemControlBlock* start;

    heapStart &= 0xFFFFFFFC;
    heapEnd &= 0xFFFFFFFC;

    heap = &sHeaps[heapIdx];

    start = (MemControlBlock*)heapStart;
    end = (MemControlBlock*)((intptr_t)heapEnd - sizeof(MemControlBlock));
    heap->size = heapEnd - heapStart;
    heap->start = start;

    *(u_long128*)start = 0;

    heap->end = end;
    heap->id = heapIdx;

    start->next = end;
    start->prev = 0;
    start->attr.bits.owner = MEM_OWNER_FREE;
    start->attr.bits.heapIdx = heapIdx;

    end_3 = heap->end;

    *(u_long128*)end_3 = 0;
    end_3->prev = (MemControlBlock*)start;
    end_3->next = NULL;
    end_3->attr.bits.owner = MEM_OWNER_END;
    end_3->attr.bits.heapIdx = heapIdx;
    heap->active = 1;
    heap->hasFreedNodes = 0;
    heap->nextFree = heap->start;
}

// Main heap: 0x00A00000-0x01B00000. Debug heap (debug mode only): 0x03000000-0x05D00000.
void Mem_Init(void) {
    sMemCurAlign = 0;
    sHeaps[0].active = 0;
    sHeaps[1].active = 0;
    Mem_SetUnk54(1);
    Mem_SetUnk55(0);

    if (gDebugMode != 0) {
        Mem_InitHeap(0, 0xA00000, 0x01B00000);
        Mem_InitHeap(1, 0x03000000, 0x05D00000);
        Mem_ClearDbgArea();
    } else {
        Mem_InitHeap(0, 0xA00000, 0x01B00000);
    }
    Mem_SetCompactCursor(sHeaps->start);
}

// Usable size of an allocation
ptrdiff_t Mem_GetSize(void* ptr) {
    MemControlBlock* mcb;

    if (ptr == NULL) {
        return 0;
    }
    mcb = PTR_TO_BLOCK(ptr);
    return (ptrdiff_t)mcb->next - (ptrdiff_t)ptr;
}

void Mem_ClearBlock(MemControlBlock* mcb, s32 val) {
    s32* quads;
    s32 numQuads = BLOCK_SIZE(mcb) / (sizeof(s32) * 4);
    quads = (s32*)BLOCK_TO_PTR(mcb);
    numQuads--;
    while (numQuads != -1) {
        quads[3] = val;
        quads[2] = val;
        quads[1] = val;
        quads[0] = val;
        quads += 4;
        numQuads -= 1;
    }
}

void Mem_SetDebugFailedAlloc(s32 flag) {
    sDebugFailedAlloc = flag;
}

void Mem_MergeFreeBlocks(Heap* heap) {
    MemControlBlock* curr;

    // Merge runs of free blocks and relink prev pointers
    for (curr = heap->start; curr->attr.bits.owner != MEM_OWNER_END; curr = curr->next) {
        while (curr->attr.bits.owner == MEM_OWNER_FREE && curr->next->attr.bits.owner == MEM_OWNER_FREE) {
            curr->next = curr->next->next;
        }
        curr->next->prev = curr;
    }

    heap->hasFreedNodes = 0;
}

void Mem_MergeFreeBlocksMain(void) {
    Mem_MergeFreeBlocks(sHeaps);
}

// Packs a return address into the 22 bit attr.bits.ra format: the word address of the jal
void Mem_SaveRA(void* ra) {
    sMemSavedRa = ((intptr_t)(ra - 8) >> 2) & 0x7FFFFF;
}

// Returns 0 if the header and its links lie inside its heap, otherwise a negative error code
s32 Mem_ValidateBlock(MemControlBlock* mcb) {
    Heap* heap;
    MemControlBlock* start;
    MemControlBlock* end;
    intptr_t next;

    heap = &sHeaps[mcb->attr.bits.heapIdx];
    start = heap->start;
    if ((intptr_t)start > (intptr_t)mcb)
        return -E_BLOCK_UNDER_HEAP;

    end = heap->end;

    if ((intptr_t)end < (intptr_t)mcb)
        return -E_BLOCK_OVER_HEAP;

    next = (intptr_t)mcb->next;
    if (next < (intptr_t)start)
        if (mcb->attr.bits.owner != MEM_OWNER_END)
            return -3;

    if ((intptr_t)end < next)
        return -E_NEXT_BLOCK_OVER_HEAP;

    if ((intptr_t)mcb->prev < (intptr_t)start && mcb->prev != 0)
        return -E_PREV_BLOCK_UNDER_HEAP;

    if ((intptr_t)end < (intptr_t)mcb->prev)
        return -E_PREV_BLOCK_OVER_HEAP;

    return 0;
}

// Same as Mem_ValidateBlock for a user pointer, with -10 / -20 for pointers outside the main heap
s32 Mem_ValidatePtr(void* ptr) {
    Heap* heap = &sHeaps[0];

    if (ptr == 0)
        return 0;
    else if ((intptr_t)ptr < (intptr_t)heap->start)
        return -E_PTR_UNDER_HEAP;
    else if ((intptr_t)ptr > (intptr_t)heap->end)
        return -E_PTR_OVER_HEAP;
    else
        return Mem_ValidateBlock(PTR_TO_BLOCK(ptr));
}

void Mem_CheckMCB(MemControlBlock* mcb) {
    s32 ret = Mem_ValidateBlock(mcb);
    if (ret != 0) {
        yiAssert(0, "SG:MemCheckMCBerr", "the MCB broken maybe.code=%d\n", ret);
    }
}

// Error report for a bad block. The message was printed by code that is stripped from the retail build.
void Mem_ReportBadBlock(MemControlBlock* arg0, char* msg, s32 arg2) {
    Mem_PrintBlock(arg0, 0);
    Mem_DumpHeap(&sHeaps[arg0->attr.bits.heapIdx]);
}

u16 Mem_CalcBlockChecksum(MemControlBlock* mcb) {
    u16* data = (u16*)BLOCK_TO_PTR(mcb);
    ptrdiff_t count = (BLOCK_SIZE(mcb)) / sizeof(u16);
    u16 sum = 0;

    for (count--; count != -1; data++, count--) {
        sum += *data;
    }
    return sum;
}

// Walks the heap, checking read-only blocks' checksums and every header's links
void Mem_CheckHeapImpl(Heap* heap) {
    MemControlBlock* next;
    MemControlBlock* cur = heap->start;

    if (heap->active) {
        for (; cur != NULL; cur = cur->next) {
            if ((cur->attr.bits.readOnly) && (cur->info.bits.checkSum != Mem_CalcBlockChecksum(cur))) {
                Mem_PrintBlock(cur, 0);
                Ch_FunctionHistoryDump();
                Mem_DumpHeap(&sHeaps[cur->attr.bits.heapIdx]);
                Mem_PrintBlock(cur, 0);
                yiAssert(0, "SgMemCheckHeap", "Memory CheckSum Error");
            }
            next = cur->next;
            if (next != NULL) {
                if (Mem_ValidateBlock(next) != 0) {
                    Mem_PrintBlock(cur, 0);
                    Ch_FunctionHistoryDump();
                    Mem_DumpHeap(&sHeaps[cur->attr.bits.heapIdx]);
                    Mem_PrintBlock(cur, 0);
                    yiAssert(0, "SgMemCheckHeap", "Memory Brocken");
                }
            }
        }
    }
}

void Mem_CheckHeap(s32 heapIdx, char* file, int line) {
    Mem_CheckHeapImpl(&sHeaps[heapIdx]);
}

static void Mem_Unused(void) {
}

// Sets the alignment (a power of two) for the next forward allocation. Each allocation resets it.
void Mem_SetAlignment(int align) {
    sMemCurAlign = align - 1;
}

// Core allocator. Searches forward from heap->nextFree, or backward from the end when fromEnd is set, for
// the first free block that fits, then splits it. Forward allocations honour sMemCurAlign by splitting off
// a free padding block first. On failure in heap 0 it asks the resource cache to release memory and
// retries. If that doesn't help it prints the failure and hits a breakpoint (unless sDebugFailedAlloc is
// set, which makes it return NULL).
// name is only used in the failure message.
#ifdef NON_MATCHING
void* Mem_AllocBlock(s32 fromEnd, s32 heapIdx, s32 owner, s32 size, s8* name) {
    s32 alignedPlusHdr;
    s32 sp10;
    Heap* heap;
    MemControlBlock* newBlock;
    MemControlBlock* cur;
    MemControlBlock* next;
    s32 rem;
    s32 aligned;
    s32 pad;
    s8* printedName;
    s32 headerSize;
    u32 blockSize;

    printedName = name;
    sp10 = size + 0xF;
masterRetry:
    heap = &sHeaps[heapIdx];
    headerSize = sizeof(MemControlBlock);
    if (printedName == NULL) {
        printedName = (s8*)sEmptyDebugName;
    }
    if (heap->hasFreedNodes != 0) {
        Mem_MergeFreeBlocks(heap);
    }
    aligned = sp10 & 0xFFFFFFF0;
    if (fromEnd != 0) {
        cur = heap->end;
        while (1) {
            while (cur->attr.bits.owner != MEM_OWNER_FREE) {
                cur = cur->prev;
                if (cur == NULL) {
                    goto fail;
                }
            }
            blockSize = BLOCK_SIZE(cur);
            rem = blockSize - aligned;
            if (rem == headerSize)
                goto found;
            if (rem > headerSize) {
                newBlock = (MemControlBlock*)((char*)cur + rem);
                memset(newBlock, 0, 0x10);
                newBlock->next = cur->next;
                newBlock->next->prev = newBlock;
                newBlock->prev = cur;
                cur->next = newBlock;
                cur = newBlock;
                goto found;
            }
            cur = cur->prev;
        }
    } else {
        cur = heap->nextFree;
        alignedPlusHdr = aligned + 0x10;
        while (1) {
            s32 align;
            while (cur->attr.bits.owner != MEM_OWNER_FREE) {
                cur = cur->next;
                if (cur == NULL)
                    goto fail;
            }
            next = cur->next;
            align = sMemCurAlign;
            pad = 0;
            blockSize = BLOCK_SIZE(cur);
            if (align != 0) {
                s32 off = (uintptr_t)(cur + 1) & align;
                if (off != 0) {
                    pad = (align + 1) - off;
                }
            }
            rem = blockSize - (aligned + pad);
            if (rem >= (int)sizeof(MemControlBlock)) {
                newBlock = (MemControlBlock*)((char*)cur + pad);
                if (pad != 0) {
                    memset(newBlock, 0, 0x10);
                    cur->next->prev = newBlock;
                    newBlock->next = cur->next;
                    newBlock->prev = cur;
                    cur->next = newBlock;
                    cur = newBlock;
                }
                if (rem == headerSize)
                    goto found;
                newBlock = (MemControlBlock*)((char*)cur + alignedPlusHdr);
                memset(newBlock, 0, 0x10);
                if (cur == heap->nextFree) {
                    heap->nextFree = newBlock;
                }
                cur->next->prev = newBlock;
                newBlock->next = cur->next;
                newBlock->prev = cur;
                cur->next = newBlock;
                newBlock->attr.bits.heapIdx = heapIdx;
                newBlock->attr.bits.owner = MEM_OWNER_FREE;
                goto found;
            }
            cur = next;
        }
    }
fail:
    if (sDebugFailedAlloc != 0) {
        sMemCurAlign = 0;
        sDebugFailedAlloc = 0;
        return NULL;
    }
    if (heapIdx == 0) {
        // Release cached resources and retry
        if (Ch_MemMove_Over() != 0) {
            goto masterRetry;
        }
        if (Cache_OverFlow() == 0) {
            goto masterRetry;
        }
    }
    DebugPrintf("%sMemory Not Enough\nfile=%s\nFunction [%s + %xh]\nsize=%d(%xh)\n",
                heapIdx ? "Debug " : sEmptyDebugName, printedName, Sg_GetFuncName(sMemSavedRa << 2),
                (sMemSavedRa << 2) - Elf_GetFuncNameFromAddr(), aligned, aligned);
    Break();
    goto masterRetry;
found:
    cur->attr.bits.owner = owner;
    cur->attr.bits.ra = sMemSavedRa;
    cur->attr.bits.heapIdx = heapIdx;
    cur->attr.bits.readOnly = 0;
    sMemCurAlign = 0;
    sDebugFailedAlloc = 0;
    return cur + 1;
}
#else
#if defined(VERSION_US_ALPHA)
INCLUDE_ASM("asm/us_alpha/nonmatchings/main/mem", Mem_AllocBlock);
#else
INCLUDE_ASM("asm/intl/nonmatchings/main/mem", Mem_AllocBlock);
#endif
#endif

// Resizes an allocation. Shrinking frees the tail in place. Growing allocates a new block with the old
// block's owner, copies the data and frees the old block. A NULL ptr allocates a new MEM_OWNER_ANY block.
// owner is unused.
void* Mem_ReallocBlock(s32 heapIdx, s32 owner, void* ptr, u32 size) {
    void* newBlock;
    u32 aligned;
    u32 blockSize;
    MemControlBlock* mcb;

    if (ptr == NULL) {
        return Mem_AllocBlock(0, heapIdx, MEM_OWNER_ANY, size, NULL);
    }
    mcb = PTR_TO_BLOCK(ptr);
    size += 0xF;
    aligned = size & ~0xF;
    Mem_CheckMCB(mcb);
    blockSize = BLOCK_SIZE(mcb);
    if (blockSize == aligned) {
        return ptr;
    }
    if (aligned < blockSize) {
        Mem_InsertFreeBlock(mcb->attr.bits.heapIdx, (MemControlBlock*)((char*)(mcb + 1) + aligned), mcb, mcb->next);
        return ptr;
    }
    newBlock = Mem_AllocBlock(0, heapIdx, mcb->attr.bits.owner, aligned, NULL);
    memcpy(newBlock, ptr, blockSize);
    Mem_FreeBlock(ptr, mcb->attr.bits.owner);
    return newBlock;
}

// Marks a block read-only and stores its checksum, which Mem_CheckHeapImpl verifies
void Mem_SetBlockReadOnly(void* ptr, s32 readOnly) {
    MemControlBlock* mcb = PTR_TO_BLOCK(ptr);

    mcb->attr.bits.readOnly = readOnly;
    if (readOnly != 0) {
        mcb->info.bits.checkSum = Mem_CalcBlockChecksum(mcb);
    }
}

// Clears the header's attributes, marking the block free. Merging happens later in Mem_MergeFreeBlocks.
// Load reorderings/regalloc
#ifdef NON_MATCHING
void Mem_MarkFree(MemControlBlock* mcb) {
    Heap* heap = &sHeaps[mcb->attr.bits.heapIdx];

    mcb->info.bits.debugIndex = 0;
    mcb->info.bits.subType = 0;
    mcb->info.bits.checkSum = 0;
    mcb->attr.bits.owner = MEM_OWNER_FREE;
    mcb->attr.bits.ra = 0;
    mcb->attr.bits.readOnly = 0;
    mcb->attr.bits.unk29 = 0;

    if ((u32)mcb < (u32)heap->nextFree) {
        heap->nextFree = mcb;
    }
    heap->hasFreedNodes = 1;
}
#else
void Mem_MarkFree(MemControlBlock* mcb);
#if defined(VERSION_US_ALPHA)
INCLUDE_ASM("asm/us_alpha/nonmatchings/main/mem", Mem_MarkFree);
#else
INCLUDE_ASM("asm/intl/nonmatchings/main/mem", Mem_MarkFree);
#endif
#endif

// Merges a free block with the free blocks around it and returns the merged block
MemControlBlock* Mem_MergeFreeNeighbours(MemControlBlock* mcb) {
    MemControlBlock* prev;

    if (mcb->attr.bits.owner != MEM_OWNER_FREE) {
        return mcb;
    }
    for (prev = mcb->prev; prev->attr.bits.owner == MEM_OWNER_FREE; prev = mcb->prev) {
        mcb = prev;
    }
    while (mcb->attr.bits.owner == MEM_OWNER_FREE && mcb->next->attr.bits.owner == MEM_OWNER_FREE) {
        mcb->next = mcb->next->next;
    }
    mcb->next->prev = mcb;
    return mcb;
}

// Writes a new free header at curBlock between prev and next, then merges it with its neighbours
void Mem_InsertFreeBlock(s32 heapIdx, MemControlBlock* curBlock, MemControlBlock* prev, MemControlBlock* next) {
    Heap* heap = &sHeaps[heapIdx];

    memset(curBlock, 0, sizeof(MemControlBlock));
    curBlock->next = next;
    curBlock->prev = prev;
    curBlock->attr.bits.heapIdx = heapIdx;
    prev->next = curBlock;
    curBlock->next->prev = (MemControlBlock*)curBlock;
    Mem_MarkFree(curBlock);
    Mem_MergeFreeNeighbours(curBlock);
    heap->hasFreedNodes = 0;
}

s32 Mem_IsFree(void* ptr) {
    MemControlBlock* mcb = PTR_TO_BLOCK(ptr);
    if (ptr == NULL) {
        return NULL;
    }
    return mcb->attr.bits.owner == MEM_OWNER_FREE;
}

// Frees an allocation. The owner must match the block's, unless the block is MEM_OWNER_ANY or owner is -1.
// Double frees and owner mismatches are reported, and the block is freed anyway.
void Mem_FreeBlock(void* ptr, s32 owner) {
    MemControlBlock* mcb = PTR_TO_BLOCK(ptr);

    if (ptr != NULL) {
        if (mcb->attr.bits.owner == MEM_OWNER_FREE) {
            Mem_ReportBadBlock(mcb, "this ptr is already free.", 0);
        }
        if ((mcb->attr.bits.owner != owner) && (mcb->attr.bits.owner != MEM_OWNER_ANY) && (owner != -1)) {
            Mem_ReportBadBlock(mcb, "memory owner is differrent", 0);
        }
        Mem_ClearDebugName(ptr);
        Mem_MarkFree(mcb);
    }
}

// Debug mode only. Writes a buffer to "host0:<fileName>" on the dev PC.
void Mem_WriteToHost(void* ptr, char* fileName) {
    if (gDebugMode != 0) {
        MemControlBlock* mcb = (MemControlBlock*)ptr - 1;
        Sg_PcWrite(ptr, (uintptr_t)mcb->next - (uintptr_t)ptr, fileName);
    }
}

// The printing was stripped from the retail build. Only the name lookup is left.
void Mem_PrintBlock(MemControlBlock* mcb, s32 index) {
    if ((mcb->attr.bits.owner != MEM_OWNER_FREE) && (mcb->attr.bits.owner != MEM_OWNER_END)) {
        Mem_GetDebugName(mcb + 1);
    }
}

// Prints every allocated block and every free block of 0x40 bytes or more
void Mem_DumpHeap(Heap* heap) {
    MemControlBlock* node = heap->start;
    s32 i;

    if (heap->hasFreedNodes) {
        Mem_MergeFreeBlocks(heap);
    }
    for (i = 0; node != NULL; i++, node = node->next) {
        if (BLOCK_SIZE(node) >= 0x40 || node->attr.bits.owner) {
            Mem_PrintBlock(node, i);
        }
    }
}

void Mem_Dump(s32 heapIdx) {
    switch (heapIdx) {
        case 1:
            Mem_DumpHeap(&sHeaps[1]);
            break;
        case 2:
            Mem_DumpHeap(&sHeaps[2]);
            break;
        default:
            Mem_DumpHeap(&sHeaps[0]);
            break;
    }

    Cache_Dump("Cache Dump");
}

// The functions below move blocks around to defragment the main heap. Callers have to update their own
// pointers to the moved data.

// Searches the main heap downward from the end for a free block bigger than ptr's block.
// Returns that free block's data pointer, or NULL if there is none above ptr.
void* Mem_FindFreeBlockAbove(void* ptr) {
    MemControlBlock* block;
    MemControlBlock* cur;

    block = PTR_TO_BLOCK(ptr);
    for (cur = sHeaps->end; cur != block; cur = cur->prev) {
        if (cur->attr.bits.owner == MEM_OWNER_FREE && BLOCK_SIZE(cur) > BLOCK_SIZE(block)) {
            return BLOCK_TO_PTR(cur);
        }
    }
    return NULL;
}

// If the next block is free, moves this block to the top of it. Returns the new data pointer, or ptr if
// the block did not move.
void* Mem_SlideUp(void* ptr) {
    MemControlBlock* mcb = (MemControlBlock*)ptr - 1;
    MemControlBlock* next = mcb->next;
    MemControlBlock* newBlock;
    MemControlBlock* prev;
    void* newPtr;
    u32 size;

    if (next->attr.bits.owner != MEM_OWNER_FREE) {
        return ptr;
    }
    size = BLOCK_SIZE(mcb);
    next = next->next;
    newPtr = (char*)next - size;
    newBlock = PTR_TO_BLOCK(newPtr);
    memmove(newBlock, mcb, size + sizeof(MemControlBlock));
    newBlock->next = next;
    newBlock->prev = mcb;
    mcb->next = newBlock;
    next->prev = newBlock;
    prev = mcb->prev;
    if (prev->attr.bits.owner == MEM_OWNER_FREE) {
        prev->next = mcb->next;
    } else {
        Mem_MarkFree(mcb);
    }
    return BLOCK_TO_PTR(newBlock);
}

// Moves ptr's block to the top of the free block freePtr (from Mem_FindFreeBlockAbove) and frees the old
// block. Returns the new data pointer.
void* Mem_MoveIntoFreeBlock(void* freePtr, void* ptr) {
    MemControlBlock* freeBlock = PTR_TO_BLOCK(freePtr);
    MemControlBlock* mcb = PTR_TO_BLOCK(ptr);
    MemControlBlock* newBlock;
    u32 len = (uintptr_t)mcb->next - (uintptr_t)mcb;
    u32 freeLen = (uintptr_t)freeBlock->next - (uintptr_t)freeBlock;

    freePtr = (char*)freePtr + (freeLen - len - sizeof(MemControlBlock));
    newBlock = freePtr;
    memmove(newBlock, mcb, len);
    Mem_MarkFree(mcb);
    newBlock->next = freeBlock->next;
    freeBlock->next->prev = newBlock;
    freeBlock->next = newBlock;
    newBlock->prev = freeBlock;
    return BLOCK_TO_PTR(newBlock);
}

// NULL resets the cursor to the start of the main heap
void Mem_SetCompactCursor(void* ptr) {
    if (ptr != NULL) {
        sCompactCursor = PTR_TO_BLOCK(ptr);
        return;
    }
    sCompactCursor = sHeaps[0].start;
}

// Moves *handle's block to a lower address: into the first big enough free block between sCompactCursor
// and the block, or else down into the free block directly below it. Updates *handle, adds the size to
// *movedBytes if given, and returns how far the data moved (0 if it didn't).
#ifdef NON_MATCHING
s32 Mem_CompactBlock(void** handle, u32* movedBytes) {
    void* ptr = *handle;
    MemControlBlock* cur = sCompactCursor;
    s32 rv;
    MemControlBlock* mcb;
    MemControlBlock* next;
    MemControlBlock* savedPrev;
    MemControlBlock* split;
    u32 sizeWithHdr;
    u32 size;

    if (ptr == NULL) {
        return 0;
    }
    mcb = PTR_TO_BLOCK(ptr);
    size = BLOCK_SIZE(mcb);
    sizeWithHdr = size + sizeof(MemControlBlock);

    // Look for an earlier free block big enough to hold this one
    for (; cur->attr.bits.owner != MEM_OWNER_END && (intptr_t)cur < (intptr_t)mcb->prev; cur = cur->next) {
        if (cur->attr.bits.owner == MEM_OWNER_FREE && size < BLOCK_SIZE(cur)) {
            next = cur->next;
            savedPrev = cur->prev;
            memmove(cur, mcb, sizeWithHdr);
            *handle = cur + 1;
            Mem_MarkFree(mcb);
            split = (MemControlBlock*)((char*)cur + sizeWithHdr);
            cur->prev = savedPrev;
            cur->next = split;
            Mem_InsertFreeBlock(0, split, cur, next);
            Mem_MergeFreeNeighbours(mcb);
            rv = (char*)*handle - (char*)ptr;
            if (movedBytes != NULL) {
                *movedBytes += size;
            }
            return rv;
        }
    }

    // Otherwise slide down into the free block directly before it
    if (mcb->prev->attr.bits.owner != MEM_OWNER_FREE) {
        return 0;
    }
    sizeWithHdr = size + sizeof(MemControlBlock);
    cur = Mem_MergeFreeNeighbours(mcb->prev);
    next = mcb->next;
    savedPrev = cur->prev;
    memmove(cur, mcb, sizeWithHdr);
    split = (MemControlBlock*)((char*)cur + sizeWithHdr);
    cur->prev = savedPrev;
    *handle = cur + 1;
    cur->next = split;
    Mem_InsertFreeBlock(0, split, cur, next);
    rv = (char*)*handle - (char*)ptr;
    if (movedBytes != NULL) {
        *movedBytes += size;
    }
    return rv;
}
#else
#if defined(VERSION_US_ALPHA)
INCLUDE_ASM("asm/us_alpha/nonmatchings/main/mem", Mem_CompactBlock);
#else
INCLUDE_ASM("asm/intl/nonmatchings/main/mem", Mem_CompactBlock);
#endif
#endif

void Mem_SetSubType(void* ptr, s32 subType) {
    MemControlBlock* mcb = PTR_TO_BLOCK(ptr);
    mcb->info.bits.subType = subType;
}

// Takes the owner minus one, like Mem_Alloc
void Mem_SetOwner(s32 ptr, s32 ownerLess1) {
    MemControlBlock* mcb = PTR_TO_BLOCK(ptr);
    mcb->attr.bits.owner = ownerLess1 + 1;
}

typedef enum AllocKind {
    /* -1 */ AllocSentinal = -1,
    /* 0 */ Alloc0,
    /* 1 */ Alloc1,
    /* 2 */ Alloc2,
    /* 3 */ Alloc3,
    /* 4 */ Alloc4,
    /* 5 */ Alloc5,
    /* 6 */ Alloc6,
    /* 7 */ AllocFree,
    /* 8 */ Alloc8,
    /* 9 */ Alloc9,
} AllocKind;

// Adds up the main heap's block sizes (headers included) into nine categories by owner, and by subType
// for owner 7. Free blocks go in sizes[7]. Returns the heap size.
s32 Mem_GetUsage(s32 sizes[8]) {
    MemControlBlock* cur = sHeaps[0].start;
    s32 i;
    s32 kind = 0;

    Mem_CheckHeap(0, "src/main/mem.c", 1071);
    for (i = 8; i >= 0; i--) {
        sizes[i] = 0;
    }
    for (; cur != NULL; cur = cur->next) {
        switch (cur->attr.bits.owner) {
            case MEM_OWNER_END:
                kind = AllocSentinal;
                break;
            case MEM_OWNER_FREE:
                kind = AllocFree;
                break;
            case MEM_OWNER_ISHI:
                kind = Alloc3;
                break;
            case MEM_OWNER_YANA:
                kind = Alloc1;
                break;
            case MEM_OWNER_SUGI:
                switch (cur->info.bits.subType) {
                    case 1:
                        kind = Alloc4;
                        break;
                    case 2:
                        kind = Alloc5;
                        break;
                    case 3:
                        kind = Alloc6;
                        break;
                    case 4:
                        kind = Alloc8;
                        break;
                    case 5:
                        kind = Alloc2;
                        break;
                    case 0:
                        kind = Alloc0;
                        break;
                }
                break;
            case MEM_OWNER_AKA:
            case MEM_OWNER_EGA:
            case MEM_OWNER_YONI:
            case MEM_OWNER_HASE:
                kind = Alloc2;
                break;
            default:
                kind = Alloc0;
                break;
        }
        if (kind != AllocSentinal) {
            sizes[kind] += (uintptr_t)cur->next - (uintptr_t)cur;
        }
    }
    return sHeaps[0].size + sizeof(MemControlBlock);
}

// Returns the largest free block in the main heap. Mode 1 merges free blocks first. Mode 2 first releases
// everything the resource cache can give back.
u32 Mem_GetLargestFree(s32 mode) {
    MemControlBlock* cur = sHeaps[0].start;
    u32 curBlockSize;
    u32 maxBlockSize = 0;

    if (mode == 1) {
        Mem_MergeFreeBlocksMain();
    } else if (mode == 2) {
        while (Ch_MemMove_Over() != 0)
            ;
        while (Cache_OverFlow() == 0)
            ;
    }
    for (; cur != NULL; cur = cur->next) {
        if (cur->attr.bits.owner == MEM_OWNER_FREE) {
            curBlockSize = BLOCK_SIZE(cur);
            maxBlockSize = (maxBlockSize < curBlockSize) ? curBlockSize : maxBlockSize;
        }
    }
    return maxBlockSize;
}
