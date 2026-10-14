#include "common.h"
#include "mem.h"

// Debug allocation name table, only present when D_00565DF0 is set
#define MEM_DEBUG_INFO ((MemDebugInfo*)0x05FB0000)
#define MEM_DEBUG_INFO_COUNT 0x800

// MemControlBlock.unk8 bits holding the block's MEM_DEBUG_INFO index
#define MCB_DEBUG_INDEX_MASK 0x07FF0000

#define BLOCK_SIZE(block) (((uintptr_t)block->next - (uintptr_t)block) - sizeof(MemControlBlock))

extern s8 D_00565DF0; 

static s32 sDebugFailedAlloc = 0;
static char* D_00307BE0[] = {
    "----", "aka ", "ega ", "toni", "yoni", "kata", "masa", "sugi",
    "nari", "hase", "thas", "sasa", "yana", "ishi", "    ", "    ",
};

static const char D_00569758[] = "";

static s32 sMemCurAlign;
static Heap sHeaps[3];
static intptr_t sMemSavedRa;
static s8 D_00580494; // Only set
static s8 D_00580495; // Only set
static MemControlBlock* D_00580498;

void func_0011C950(s32, MemControlBlock*, MemControlBlock*, MemControlBlock*);
void Mem_SaveRA(void*);
void func_0011CB08(MemControlBlock*, s32);
void func_0011CB30(Heap*);

void func_0011CDC0(void*);

extern void* func_002EEC5C(void*, void*, int);
extern void func_00110CE0(char*);

void Mem_ClearDbgArea(void) {
    s32 next;
    MemDebugInfo* curInfo;

    if (D_00565DF0 != 0) {
        next = MEM_DEBUG_INFO_COUNT - 1;
        curInfo = &MEM_DEBUG_INFO[MEM_DEBUG_INFO_COUNT - 1];
        do {
            curInfo->name[0] = 0;
            next = next + -1;
            curInfo--;
        } while (-1 < next);
    }
}

void func_0011BA60(void* arg0) {
    MemControlBlock* mcb;

    mcb = arg0 - sizeof(MemControlBlock);
    if ((D_00565DF0 != 0) && (arg0 != NULL)) {
        if (mcb->unk8.raw & MCB_DEBUG_INDEX_MASK) {
            MEM_DEBUG_INFO[(mcb->unk8.raw >> 16) & 0x7FF].name[0] = 0;
            mcb->unk8.raw = (u32)(mcb->unk8.raw & ~MCB_DEBUG_INDEX_MASK);
        }
    }
}


MemDebugInfo* func_0011BAB8(void* ptr) {
    MemControlBlock* mcb = ptr - sizeof(MemControlBlock);
    MemDebugInfo* arr = MEM_DEBUG_INFO;

    if (D_00565DF0 == 0) {
        return (MemDebugInfo*)&D_00569758;
    }

    if (ptr == NULL) {
        return (MemDebugInfo*)&D_00569758;
    }

    if ((mcb->unk8.raw & MCB_DEBUG_INDEX_MASK) == 0) {
        return (MemDebugInfo*)&D_00569758;
    }
    return &arr[(mcb->unk8.raw >> 16) & 0x7FF];
}

int sprintf(char* str, const char* fmt, ...);
void* memset(void*, s32, s32);
int strlen(char*);
int strncpy(char*, char*, int);
void* memmove(void* dst, const void* src, unsigned int size);
// printf
int func_002EEED0(const char* fmt, ...);

// Insert debug string
void func_0011BB08(void* ptr, char* file, int line) {
    MemControlBlock* mcb = (MemControlBlock*)ptr - 1;
    MemDebugInfo* info = MEM_DEBUG_INFO;
    int index = 0;
    char name[0x30];
    s32 i;

    if (D_00565DF0 == 0 || ptr == NULL) {
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

        if (mcb->unk8.raw & MCB_DEBUG_INDEX_MASK) {
            memset((void*)(((mcb->unk8.raw >> 16) & 0x7FF) * sizeof(MemDebugInfo) + (u32)info), 0,
                   sizeof(MemDebugInfo));
            strncpy(info[mcb->unk8.bits.debugIndex].name, name, sizeof(info->name) - 1);
            info[mcb->unk8.bits.debugIndex].name[sizeof(info->name) - 1] = '\0';
            index = (mcb->unk8.raw >> 16) & 0x7FF;
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

    mcb->unk8.bits.debugIndex = index;
}


void Mem_SetBlockRA(void* ptr) {
    MemControlBlock* mcb = ptr - sizeof(MemControlBlock);
    if (ptr != 0) {
        void* ra;
        GetRa(&ra);
        Mem_SaveRA(ra);
        mcb->unkC.bits2.ra = sMemSavedRa;
    }
}

void func_0011BD70(s8 arg0) {
    D_00580494 = arg0;
}

void func_0011BD80(s8 arg0) {
    D_00580495 = arg0;
}

void Mem_InitHeap(u32 heapIdx, intptr_t heapTop, intptr_t heapEnd) {
    Heap* heap;
    MemControlBlock* end;
    MemControlBlock* end_3;
    MemControlBlock* top;

    heapTop &= 0xFFFFFFFC;
    heapEnd &= 0xFFFFFFFC;

    heap = &sHeaps[heapIdx];

    top = (MemControlBlock*)heapTop;
    end = (MemControlBlock*)((intptr_t)heapEnd - sizeof(MemControlBlock));
    heap->size = heapEnd - heapTop;
    heap->top = top;

    *(u_long128*)top = 0;

    heap->end = end;
    heap->id = heapIdx;

    top->next = end;
    top->prev = 0;
    top->unkC.bits2.a = 0;
    top->unkC.bits2.heapIdx = heapIdx;

    end_3 = heap->end;

    *(u_long128*)end_3 = 0;
    end_3->prev = (MemControlBlock*)top;
    end_3->next = NULL;
    end_3->unkC.bits2.a = 0xF;
    end_3->unkC.bits2.heapIdx = heapIdx;
    heap->active = 1;
    heap->hasFreedNodes = 0;
    heap->nextFree = heap->top;
}


void func_0011BE48(void) {
    sMemCurAlign = 0;
    sHeaps[0].active = 0;
    sHeaps[1].active = 0;
    func_0011BD70(1);
    func_0011BD80(0);

    if (D_00565DF0 != 0) {
        Mem_InitHeap(0, 0xA00000, 0x01B00000);
        Mem_InitHeap(1, 0x03000000, 0x05D00000);
        Mem_ClearDbgArea();
    } else {
        Mem_InitHeap(0, 0xA00000, 0x01B00000);
    }
    func_0011CDC0(sHeaps->top);
}

ptrdiff_t func_0011BEE0(void* arg0) {
    MemControlBlock* mcb;
    if (arg0 == NULL) {
        return 0;
    }
    mcb = (MemControlBlock*)arg0 - 1;
    return (ptrdiff_t)mcb->next - (ptrdiff_t)arg0;
}

void Mem_ClearBlock(MemControlBlock* mcb, s32 val) {
    s32* quads;
    s32 numQuads = BLOCK_SIZE(mcb) / sizeof(s32) * 4;
    quads = (s32*)(mcb + 1);
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

void func_0011BF50(Heap* heap) {
    MemControlBlock* curr;

    // Merge runs of free blocks and relink prev pointers
    for (curr = heap->top; curr->unkC.bits2.a != 0xF; curr = curr->next) {
        while (curr->unkC.bits2.a == 0 && curr->next->unkC.bits2.a == 0) {
            curr->next = curr->next->next;
        }
        curr->next->prev = curr;
    }

    heap->hasFreedNodes = 0;
}

void func_0011BFD0(void) {
    func_0011BF50(sHeaps);
}

void Mem_SaveRA(void* ra) {
    sMemSavedRa = ((intptr_t)(ra - 8) >> 2) & 0x7FFFFF;
}

// Validate node?
s32 func_0011C000(MemControlBlock* mcb) {
    Heap* heap;
    MemControlBlock* top;
    MemControlBlock* end;
    intptr_t next;

    heap = &sHeaps[mcb->unkC.bits2.heapIdx];
    top = heap->top;
    if ((intptr_t)top > (intptr_t)mcb)
        return -1;

    end = heap->end;

    if ((intptr_t)end < (intptr_t)mcb)
        return -2;

    next = (intptr_t)mcb->next;
    if (next < (intptr_t)top)
        if (mcb->unkC.bits2.a != 0xF)
            return -3;

    if ((intptr_t)end < next)
        return -4;

    if ((intptr_t)mcb->prev < (intptr_t)top && mcb->prev != 0)
        return -5;

    if ((intptr_t)end < (intptr_t)mcb->prev)
        return -6;

    return 0;
}

s32 func_0011C0B0(void* ptr) {
    Heap* heap = &sHeaps[0];

    if (ptr == 0)
        return 0;
    else if ((intptr_t)ptr < (intptr_t)heap->top)
        return -10;
    else if ((intptr_t)ptr > (intptr_t)heap->end)
        return -20;
    else
        return func_0011C000((MemControlBlock*)ptr - 1);
}

void func_0011C100(MemControlBlock* arg0) {
    s32 ret = func_0011C000(arg0);
    if (ret != 0) {
        func_00179058(0, "SG:MemCheckMCBerr", "the MCB broken maybe.code=%d\n", ret);
    }
}

void func_0011C140(MemControlBlock* arg0, char* arg1, s32 arg2) {
    func_0011CB08(arg0, 0);
    func_0011CB30(&sHeaps[arg0->unkC.bits2.heapIdx]);
}

u16 Mem_CalcBlockChecksum(MemControlBlock* mcb) {
    u16* data = (u16*)(mcb + 1);
    ptrdiff_t count = (BLOCK_SIZE(mcb)) / sizeof(u16);
    u16 sum = 0;

    for (count--; count != -1; data++, count--) {
        sum += *data;
    }
    return sum;
}

void func_0011C1D8(Heap* heap) {
    MemControlBlock* next;
    MemControlBlock* cur = heap->top;

    if (heap->active) {
        for (; cur != NULL; cur = cur->next) {
            if ((cur->unkC.bits2.readOnly) && (cur->unk8.bits.checkSum != Mem_CalcBlockChecksum(cur))) {
                func_0011CB08(cur, 0);
                func_00103828();
                func_0011CB30(&sHeaps[cur->unkC.bits2.heapIdx]);
                func_0011CB08(cur, 0);
                func_00179058(0, "SgMemCheckHeap", "Memory CheckSum Error");
            }
            next = cur->next;
            if (next != NULL) {
                if (func_0011C000(next) != 0) {
                    func_0011CB08(cur, 0);
                    func_00103828();
                    func_0011CB30(&sHeaps[cur->unkC.bits2.heapIdx]);
                    func_0011CB08(cur, 0);
                    func_00179058(0, "SgMemCheckHeap", "Memory Brocken");
                }
            }
        }
    }
}

void func_0011C328(s32 heapIdx, char* f, int l) {
    func_0011C1D8(&sHeaps[heapIdx]);
}

void func_0011C340(void) {
}

void Mem_SetAlignment(int align) {
    sMemCurAlign = align - 1;
}

#ifdef NON_MATCHING
void* func_0011C358(s32 fromEnd, s32 heapIdx, s32 type, s32 size, s8* name) {
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
    heap = &D_00580448[heapIdx];
    headerSize = sizeof(MemControlBlock);
    if (printedName == NULL) {
        printedName = &D_00569758.name[0];
    }
    if (heap->hasFreedNodes != 0) {
        func_0011BF50(heap);
    }
    aligned = sp10 & 0xFFFFFFF0;
    if (fromEnd != 0) {
        cur = heap->end;
        while (1) {
            while (cur->unkC.bits2.a != 0) {
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
            while (cur->unkC.bits2.a != 0) {
                cur = cur->next;
                if (cur == NULL)
                    goto fail;
            }
            next = cur->next;
            align = D_00580440;
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
                newBlock->unkC.bits2.heapIdx = heapIdx;
                newBlock->unkC.bits2.a = 0;
                goto found;
            }
            cur = next;
        }
    }
fail:
    if (D_00307BD8 != 0) {
        D_00580440 = 0;
        D_00307BD8 = 0;
        return NULL;
    }
    if (heapIdx == 0) {
        if (func_00103628() != 0) {
            goto masterRetry;
        }
        if (func_00110D70() == 0) {
            goto masterRetry;
        }
    }
    func_00120108("%sMemory Not Enough\nfile=%s\nFunction [%s + %xh]\nsize=%d(%xh)\n", heapIdx ? "Debug " : D_00569758.name,
                  printedName, func_0011A330(D_00580490 << 2), (D_00580490 << 2) - func_0011A3C8(),
                  aligned, aligned);
    Break();
    goto masterRetry;
found:
    cur->unkC.bits2.a = type;
    cur->unkC.bits2.ra = D_00580490;
    cur->unkC.bits2.heapIdx = heapIdx;
    cur->unkC.bits2.c = 0;
    D_00580440 = 0;
    D_00307BD8 = 0;
    return cur + 1;
}
#else
INCLUDE_ASM("asm/nonmatchings/main/mem", func_0011C358);
#endif


void* func_0011C6E0(s32 heapIdx, s32 arg1, void* ptr, u32 size) {
    void* newBlock;
    u32 aligned;
    u32 blockSize;
    MemControlBlock* mcb;

    if (ptr == NULL) {
        return func_0011C358(0, heapIdx, 0xE, size, NULL);
    }
    mcb = (MemControlBlock*)ptr - 1;
    size += 0xF;
    aligned = size & ~0xF;
    func_0011C100(mcb);
    blockSize = BLOCK_SIZE(mcb);
    if (blockSize == aligned) {
        return ptr;
    }
    if (aligned < blockSize) {
        func_0011C950(mcb->unkC.bits2.heapIdx, (MemControlBlock*)((char*)(mcb + 1) + aligned), mcb, mcb->next);
        return ptr;
    }
    newBlock = func_0011C358(0, heapIdx, mcb->unkC.bits2.a, aligned, NULL);
    func_002EEBAC(newBlock, ptr, blockSize);
    func_0011CA28(ptr, mcb->unkC.bits2.a);
    return newBlock;
}

void Mem_SetBlockReadOnly(void* ptr, s32 readOnly) {
    MemControlBlock* mcb = (MemControlBlock*)ptr - 1;

    mcb->unkC.bits2.readOnly = readOnly;
    if (readOnly != 0) {
        mcb->unk8.bits.checkSum = Mem_CalcBlockChecksum(mcb);
    }
}

// Load reorderings/regalloc
#ifdef NON_MATCHING
void func_0011C838(MemControlBlock* mcb) {
    Heap* heap = &D_00580448[mcb->unkC.bits2.heapIdx];

    mcb->unk8.bits.debugIndex = 0;
    mcb->unk8.bits.b27 = 0;
    mcb->unkC.bits2.a = 0;
    mcb->unkC.bits2.ra = 0;
    mcb->unkC.bits2.c = 0;
    mcb->unkC.bits2.e = 0;
    mcb->unk8.bits.b0 = 0;

    if ((u32)mcb < (u32)heap->nextFree) {
        heap->nextFree = mcb;
    }
    heap->hasFreedNodes = 1;
}
#else
void func_0011C838(MemControlBlock* mcb);
INCLUDE_ASM("asm/nonmatchings/main/mem", func_0011C838);
#endif

MemControlBlock* func_0011C8B8(MemControlBlock* mcb) {
    MemControlBlock* prev;

    if (mcb->unkC.bits2.a != 0) {
        return mcb;
    }
    for (prev = mcb->prev; prev->unkC.bits2.a == 0; prev = mcb->prev) {
        mcb = prev;
    }
    while (mcb->unkC.bits2.a == 0 && mcb->next->unkC.bits2.a == 0) {
        mcb->next = mcb->next->next;
    }
    mcb->next->prev = mcb;
    return mcb;
}

void func_0011C950(s32 heapIdx, MemControlBlock* curBlock, MemControlBlock* prev, MemControlBlock* next) {
    Heap* heap = &sHeaps[heapIdx];

    memset(curBlock, 0, sizeof(MemControlBlock));
    curBlock->next = next;
    curBlock->prev = prev;
    curBlock->unkC.bits2.heapIdx = heapIdx;
    prev->next = curBlock;
    curBlock->next->prev = (MemControlBlock*)curBlock;
    func_0011C838(curBlock);
    func_0011C8B8(curBlock);
    heap->hasFreedNodes = 0;
}

s32 func_0011CA08(void* arg0) {
    MemControlBlock* mcb = (MemControlBlock*)(arg0 - sizeof(MemControlBlock));
    if (arg0 == NULL) {
        return NULL;
    }
    return mcb->unkC.bits2.a == 0;
}

void func_0011CA28(void* ptr, s32 type) {
    MemControlBlock* mcb;

    mcb = ptr - sizeof(MemControlBlock);
    if (ptr != NULL) {
        if (mcb->unkC.bits2.a == 0) {
            func_0011C140(mcb, "this ptr is already free.", 0);
        }
        if ((mcb->unkC.bits2.a != type) && (mcb->unkC.bits2.a != 0xE) && (type != -1)) {
            func_0011C140(mcb, "memory owner is differrent", 0);
        }
        func_0011BA60(ptr);
        func_0011C838(mcb);
    }
}

void func_0011CAE0(void* ptr, s32 arg1) {
    if (D_00565DF0 != 0) {
        MemControlBlock* mcb = (MemControlBlock*)ptr - 1;
        func_0011F190(ptr, (uintptr_t)mcb->next - (uintptr_t)ptr, arg1);
    }
}

void func_0011CB08(MemControlBlock* mcb, s32 arg1) {
    if ((mcb->unkC.bits2.a != 0) && (mcb->unkC.bits2.a != 0xF)) {
        func_0011BAB8(mcb + 1);
    }
}

void func_0011CB30(Heap* heap) {
    MemControlBlock* node = heap->top;
    s32 i;

    if (heap->hasFreedNodes) {
        func_0011BF50(heap);
    }
    for (i = 0; node != NULL; i++, node = node->next) {
        if (BLOCK_SIZE(node) >= 0x40 || node->unkC.bits2.a) {
            func_0011CB08(node, i);
        }
    }
}

void func_0011CBB8(s32 heapIdx) {
    switch (heapIdx) {
        case 1:
            func_0011CB30(&sHeaps[1]);
            break;
        case 2:
            func_0011CB30(&sHeaps[2]);
            break;
        default:
            func_0011CB30(&sHeaps[0]);
            break;
    }

    func_00110CE0("Cache Dump");
}

void* func_0011CC18(void* arg0) {
    MemControlBlock* block;
    MemControlBlock* cur;

    block = arg0 - sizeof(MemControlBlock);
    for (cur = sHeaps->end; cur != block; cur = cur->prev) {
        if (cur->unkC.bits2.a == 0 && BLOCK_SIZE(cur) > BLOCK_SIZE(block)) {
            return cur + 1;
        }
    }
    return NULL;
}

void* func_0011CC78(void* ptr) {
    MemControlBlock* mcb = (MemControlBlock*)ptr - 1;
    MemControlBlock* next = mcb->next;
    MemControlBlock* newBlock;
    MemControlBlock* prev;
    void* newPtr;
    u32 size;

    if (next->unkC.bits2.a != 0) {
        return ptr;
    }
    size = BLOCK_SIZE(mcb);
    next = next->next;
    newPtr = (char*)next - size;
    newBlock = (MemControlBlock*)newPtr - 1;
    func_002EEC5C(newBlock, mcb, size + sizeof(MemControlBlock));
    newBlock->next = next;
    newBlock->prev = mcb;
    mcb->next = newBlock;
    next->prev = newBlock;
    prev = mcb->prev;
    if (prev->unkC.bits2.a == 0) {
        prev->next = mcb->next;
    } else {
        func_0011C838(mcb);
    }
    return newBlock + 1;
}

void* func_0011CD30(void* freePtr, void* ptr) {
    MemControlBlock* freeBlock = (MemControlBlock*)freePtr - 1;
    MemControlBlock* mcb = (MemControlBlock*)ptr - 1;
    MemControlBlock* newBlock;
    u32 len = (uintptr_t)mcb->next - (uintptr_t)mcb;
    u32 freeLen = (uintptr_t)freeBlock->next - (uintptr_t)freeBlock;

    freePtr = (char*)freePtr + (freeLen - len - sizeof(MemControlBlock));
    newBlock = freePtr;
    func_002EEC5C(newBlock, mcb, len);
    func_0011C838(mcb);
    newBlock->next = freeBlock->next;
    freeBlock->next->prev = newBlock;
    freeBlock->next = newBlock;
    newBlock->prev = freeBlock;
    return newBlock + 1;
}

void func_0011CDC0(void* arg0) {
    if (arg0 != NULL) {
        D_00580498 = arg0 - sizeof(MemControlBlock);
        return;
    }
    D_00580498 = sHeaps[0].top;
}

#ifdef NON_MATCHING
s32 func_0011CDE8(void** handle, u32* movedBytes) {
    void* ptr = *handle;
    MemControlBlock* cur = D_00580498;
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
    mcb = (MemControlBlock*)ptr - 1;
    size = BLOCK_SIZE(mcb);
    sizeWithHdr = size + sizeof(MemControlBlock);

    // Look for an earlier free block big enough to hold this one
    for (; cur->unkC.bits2.a != 0xF && (intptr_t)cur < (intptr_t)mcb->prev; cur = cur->next) {
        if (cur->unkC.bits2.a == 0 && size < BLOCK_SIZE(cur)) {
            next = cur->next;
            savedPrev = cur->prev;
            func_002EEC5C(cur, mcb, sizeWithHdr);
            *handle = cur + 1;
            func_0011C838(mcb);
            split = (MemControlBlock*)((char*)cur + sizeWithHdr);
            cur->prev = savedPrev;
            cur->next = split;
            func_0011C950(0, split, cur, next);
            func_0011C8B8(mcb);
            rv = (char*)*handle - (char*)ptr;
            if (movedBytes != NULL) {
                *movedBytes += size;
            }
            return rv;
        }
    }

    // Otherwise slide down into the free block directly before it
    if (mcb->prev->unkC.bits2.a != 0) {
        return 0;
    }
    sizeWithHdr = size + sizeof(MemControlBlock);
    cur = func_0011C8B8(mcb->prev);
    next = mcb->next;
    savedPrev = cur->prev;
    memmove(cur, mcb, sizeWithHdr);
    split = (MemControlBlock*)((char*)cur + sizeWithHdr);
    cur->prev = savedPrev;
    *handle = cur + 1;
    cur->next = split;
    func_0011C950(0, split, cur, next);
    rv = (char*)*handle - (char*)ptr;
    if (movedBytes != NULL) {
        *movedBytes += size;
    }
    return rv;
}
#else
INCLUDE_ASM("asm/nonmatchings/main/mem", func_0011CDE8);
#endif

void func_0011CFB8(void* arg0, s32 b27) {
    MemControlBlock* mcb = (MemControlBlock*)arg0 - 1;
    mcb->unk8.bits.b27 = b27;
}

void func_0011CFE0(s32 arg0, s32 a) {
    MemControlBlock* mcb = (MemControlBlock*)arg0 - 1;
    mcb->unkC.bits2.a = a + 1;
}

s32 func_0011D008(s32* sizes) {
    MemControlBlock* cur = sHeaps[0].top;
    s32 i;
    s32 kind = 0;

    func_0011C328(0, "src/main/mem.c", 1071);
    for (i = 8; i >= 0; i--) {
        sizes[i] = 0;
    }
    for (; cur != NULL; cur = cur->next) {
        switch (cur->unkC.bits2.a) {
            case 15:
                kind = -1;
                break;
            case 0:
                kind = 7;
                break;
            case 13:
                kind = 3;
                break;
            case 12:
                kind = 1;
                break;
            case 7:
                switch (cur->unk8.bits.b27) {
                    case 1:
                        kind = 4;
                        break;
                    case 2:
                        kind = 5;
                        break;
                    case 3:
                        kind = 6;
                        break;
                    case 4:
                        kind = 8;
                        break;
                    case 5:
                        kind = 2;
                        break;
                    case 0:
                        kind = 0;
                        break;
                }
                break;
            case 1:
            case 2:
            case 4:
            case 9:
                kind = 2;
                break;
            default:
                kind = 0;
                break;
        }
        if (kind != -1) {
            sizes[kind] += (uintptr_t)cur->next - (uintptr_t)cur;
        }
    }
    return sHeaps[0].size + sizeof(MemControlBlock);
}

u32 func_0011D178(s32 arg0) {
    MemControlBlock* cur;
    u32 curBlockSize;
    u32 maxBlockSize;

    cur = sHeaps[0].top;
    maxBlockSize = 0;
    if (arg0 == 1) {
        func_0011BFD0();
    } else if (arg0 == 2) {
        while (func_00103628() != 0)
            ;
        while (func_00110D70() == 0)
            ;
    }
    for (; cur != NULL; cur = cur->next) {
        if (cur->unkC.bits2.a == 0) {
            curBlockSize = BLOCK_SIZE(cur);
            maxBlockSize = (maxBlockSize < curBlockSize) ? curBlockSize : maxBlockSize;
        }
    }
    return maxBlockSize;
}
