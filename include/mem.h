#ifndef MEM_H
#define MEM_H

typedef unsigned int u_long128 __attribute__((mode(TI)));

// Block owner IDs (MemControlBlock.attr.bits.owner).
// Callers of Mem_Alloc and friends pass the owner minus one; the stored value indexes sMemOwnerNames in mem.c.
// These defines should only be used inside the allocate. External users should use the enum `AllocOwner`
#define MEM_OWNER_FREE 0x0 // Block is free
#define MEM_OWNER_AKA 0x1
#define MEM_OWNER_EGA 0x2
#define MEM_OWNER_TONI 0x3
#define MEM_OWNER_YONI 0x4
#define MEM_OWNER_KATA 0x5
#define MEM_OWNER_MASA 0x6
#define MEM_OWNER_SUGI 0x7
#define MEM_OWNER_NARI 0x8
#define MEM_OWNER_HASE 0x9
#define MEM_OWNER_THAS 0xA
#define MEM_OWNER_SASA 0XB
#define MEM_OWNER_YANA 0xC
#define MEM_OWNER_ISHI 0xD
#define MEM_OWNER_ANY 0xE // malloc-style allocations. Freeing one skips the owner check.
#define MEM_OWNER_END 0xF // Sentinel block at the end of each heap

// Allocation owners for use by the users outside the allocator
typedef enum AllocOwner {
    AllocOwnerAka = MEM_OWNER_AKA - 1,
    AllocOwnerEga = MEM_OWNER_EGA - 1,
    AllocOwnerToni = MEM_OWNER_TONI - 1,
    AllocOwnerYoni = MEM_OWNER_YONI - 1,
    AllocOwnerKata = MEM_OWNER_KATA - 1,
    AllocOwnerMasa = MEM_OWNER_MASA - 1,
    AllocOwnerSugi = MEM_OWNER_SUGI - 1,
    AllocOwnerNari = MEM_OWNER_NARI - 1,
    AllocOwnerHase = MEM_OWNER_HASE - 1,
    AllocOwnerThas = MEM_OWNER_THAS - 1,
    AllocOwnerSasa = MEM_OWNER_SASA - 1,
    AllocOwnerYana = MEM_OWNER_YANA - 1,
    AllocOwnerIshi = MEM_OWNER_ISHI - 1
} AllocOwner;

// Errors returned by Mem_ValidateBlock
// Under refers to having anddress before the `start`
// Over refers to having an address after the `end`
#define E_BLOCK_UNDER_HEAP 1
#define E_BLOCK_OVER_HEAP 2
#define E_NEXT_BLOCK_OVER_HEAP 4

#define E_PREV_BLOCK_UNDER_HEAP 5
#define E_PREV_BLOCK_OVER_HEAP 6

#define E_PTR_UNDER_HEAP 10
#define E_PTR_OVER_HEAP 20

// Header placed in front of every heap allocation. Blocks form a doubly linked list in address order,
// and a block's size is the distance to the next header.
typedef struct MemControlBlock_t {
    /* 0x00 */ struct MemControlBlock_t* next;
    /* 0x04 */ struct MemControlBlock_t* prev;
    /* 0x08 */ union {
        struct {
            unsigned int checkSum : 16;   // Set when `readOnly` is set
            unsigned int debugIndex : 11; // Index into MEM_DEBUG_INFO, 0 when the block has no name
            unsigned int subType : 5;     // Further splits the owner 7 statistics in Mem_GetUsage
        } bits;
        unsigned int raw;
    } info;
    /* 0x0C */ union {
        struct {
            unsigned int owner : 4;    // MEM_OWNER_* or a caller's owner ID
            unsigned int ra : 22;      // Word address of the allocating call, from Mem_SaveRA
            unsigned int readOnly : 1; // Calculates a checksum when set.
            unsigned int heapIdx : 2;
            unsigned int unk29 : 3; // Only ever cleared
        } bits;
        unsigned raw;
    } attr;
} MemControlBlock; // size = 0x10

typedef struct {
    /* 0x00 */ char hasFreedNodes; // Set when blocks were freed since the last Mem_MergeFreeBlocks
    /* 0x01 */ char active;
    /* 0x02 */ char unk2;
    /* 0x03 */ char unk3;
    /* 0x04 */ unsigned int id;
    /* 0x08 */ unsigned int size;
    /* 0x0C */ MemControlBlock* start;    // First block
    /* 0x10 */ MemControlBlock* end;      // MEM_OWNER_END sentinel block
    /* 0x14 */ MemControlBlock* nextFree; // Lowest block that may be free; forward searches start here
} Heap;                                   // size = 0x18

// Debug name ("file:line") for an allocation
typedef struct MemDebugInfo {
    /* 0x00 */ char name[0x20];
} MemDebugInfo; // size = 0x20

void Mem_SetDebugName(void* ptr, char* file, int line);
void* Mem_AllocBlock(s32 fromEnd, s32 heapIdx, s32 owner, s32 size, s8* name);
void* Mem_ReallocBlock(int heapIdx, int owner, void* ptr, unsigned int size);
void Mem_FreeBlock(void* ptr, s32 owner);

static inline void GetRa(void** ra) {
    asm("move $15, %0\n\t"
        "sw   $31, 0($15)\n\t"
        :
        : "r"(ra)
        : "t7");
}

static inline void Break(void) {
    asm("break 0\n\t");
}

#endif
