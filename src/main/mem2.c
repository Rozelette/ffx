// Public allocation wrappers around the allocator in mem.c.
//
// Owner IDs: most wrappers take the owner minus one and add one before storing it. The *Default
// wrappers use owner 6 (stored as 7). Mem_Malloc / Mem_FreeAny use MEM_OWNER_ANY.
//
// Each wrapper calls GetRa + Mem_SaveRA first, so the block records its caller's address.
//
// func_0011D718 (a realloc wrapper for owner 0xD) also belongs to this file. It is still in asm/01D718.s.

#include "common.h"
#include "mem.h"

// TODO fix the padding
static void foo() {
}

// This can't be in the header because this file assume it returns void* (or was implicitly called)
void* Mem_SaveRA(void*);
// libc
int printf(const char* fmt, ...);

// Allocates with the start of the data aligned to align (a power of two). If the first try isn't aligned,
// it frees it, allocates a filler block to push the next allocation to an aligned address, allocates again
// and frees the filler. The debug prints are left over from developing this.
void* Mem_AllocAligned(s32 size, s32 align, s32 ownerLess1, char* file, s32 line) {
    void* newBlock;
    void* ptr;
    void* ra;
    s32 tmpSize;
    s32 mask;
    s32 owner;
    s32 alignMask;

    alignMask = align - 1;
    GetRa(&ra);
    owner = ownerLess1 + 1;
    Mem_SaveRA(ra);
    ptr = Mem_AllocBlock(0, 0, owner, size, file);
    mask = (s32)ptr & alignMask;
    if (mask != 0) {
        tmpSize = (alignMask - mask) - 0xF;
        printf("size=%x ptr=%x align=%x mask=%x tmpsize=%x ", size, ptr, alignMask, mask, tmpSize);
        Mem_FreeBlock(ptr, owner);
        newBlock = Mem_AllocBlock(0, 0, owner, tmpSize, NULL);
        //! @bug Mem_Dump takes a heap index, but none is passed. a0 holds whatever Mem_AllocBlock left in it.
        Mem_Dump();
        ptr = Mem_AllocBlock(0, 0, owner, size, file);
        Mem_FreeBlock(newBlock, owner);
        printf("result=%x", ptr);
        printf("\n");
    }
    Mem_SetDebugName(ptr, file, line);
    return ptr;
}

// Main allocation function. The block's debug name is "file:line".
void* Mem_Alloc(s32 size, s32 ownerLess1, char* file, s32 line) {
    char str[0x400];
    void* ptr;
    void* ra;

    GetRa(&ra);
    Mem_SaveRA(ra);
    sprintf(str, "%s:%d", file != NULL ? file : "", line);
    ptr = Mem_AllocBlock(0, 0, ownerLess1 + 1, size, str);
    Mem_SetDebugName(ptr, file, line);
    return ptr;
}

extern s8 D_00565DF0;
// Allocates from the debug heap. Returns NULL outside debug mode.
void* Mem_AllocDebug(s32 size, s32 ownerLess1, char* file, s32 line) {
    void* ptr;
    void* ra;
    if (D_00565DF0 == 0) {
        return NULL;
    }
    GetRa(&ra);
    Mem_SaveRA(ra);
    ptr = Mem_AllocBlock(0, 1, ownerLess1 + 1, size, NULL);
    Mem_SetDebugName(ptr, file, line);
    return ptr;
}

// Like Mem_Alloc, but takes the highest free block that fits (allocates from the end of the heap)
void* Mem_AllocFromEnd(s32 size, s32 ownerLess1, char* file, s32 line) {
    char str[0x400];
    void* ptr;
    void* ra;

    GetRa(&ra);
    Mem_SaveRA(ra);
    sprintf(str, "%s:%d", file != NULL ? file : "", line);
    ptr = Mem_AllocBlock(1, 0, ownerLess1 + 1, size, str);
    Mem_SetDebugName(ptr, file, line);
    return ptr;
}

void* Mem_Realloc(void* ptr, u32 size, s32 owner) {
    return Mem_ReallocBlock(0, owner, ptr, size);
}

void Mem_Free(void* arg0, s32 ownerLess1) {
    Mem_FreeBlock(arg0, ownerLess1 + 1);
}

void* Mem_AllocDefault(s32 arg0) {
    return Mem_Alloc(arg0, 6, "src/main/mem2.c", 0x6D);
}

void* Mem_AllocDebugDefault(int len) {
    return Mem_AllocDebug(len, 6, "src/main/mem2.c", 0x71);
}

void Mem_FreeDefault(void* arg0) {
    Mem_Free(arg0, 6);
}

void* Mem_ReallocDebugDefault(void* ptr, u32 size) {
    return Mem_ReallocBlock(1, 6, ptr, size);
}

void* Mem_ReallocDefault(void* ptr, u32 size) {
    return Mem_ReallocBlock(0, 6, ptr, size);
}

// strdup into the debug heap
void* Mem_StrDupDebug(char* arg0) {
    return strcpy(Mem_AllocDebugDefault(strlen(arg0) + 1), arg0);
}

extern void* sMem2SavedRA;

// malloc replacement: MEM_OWNER_ANY, no debug name
void* Mem_Malloc(s32 size) {
    GetRa(&sMem2SavedRA);

    Mem_SaveRA(sMem2SavedRA);
    return Mem_AllocBlock(0, 0, MEM_OWNER_ANY, size, NULL);
}

void Mem_FreeAny(void* ptr) {
    Mem_FreeBlock(ptr, MEM_OWNER_ANY);
}

// strdup, stored as MEM_OWNER_ANY
void* Mem_StrDup(char* arg0) {
    return strcpy(Mem_Alloc(strlen(arg0) + 1, MEM_OWNER_ANY - 1, "src/main/mem2.c", 0x99), arg0);
}

void* Mem_ReallocOwnerD(void* ptr, s32 size) {
    return Mem_ReallocBlock(0, 0xD, ptr, size);
}