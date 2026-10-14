#ifndef MEM_H
#define MEM_H

typedef unsigned int u_long128 __attribute__((mode(TI)));

// Header placed in front of every heap allocation
typedef struct MemControlBlock_t {
    /* 0x00 */ struct MemControlBlock_t* next;
    /* 0x04 */ struct MemControlBlock_t* prev;
    /* 0x08 */ union { // TODO bitset or int?
        struct {
            unsigned int b0 : 16;
            unsigned int debugIndex : 11;
            unsigned int b27 : 5;
        } bits;
        unsigned int raw;
    } unk8;
    /* 0x0C */ union {
        struct {
            unsigned int a : 4;
            unsigned int b : 22;
            unsigned int c : 1;
            unsigned int heapIdx : 2;
            unsigned int e : 3;
        } bits2;
        unsigned raw2;
    } unkC;
} MemControlBlock; // size = 0x10

typedef struct {
    /* 0x00 */ char hasFreedNodes;
    /* 0x01 */ char active;
    /* 0x02 */ char unk2;
    /* 0x03 */ char unk3;
    /* 0x04 */ unsigned int id;
    /* 0x08 */ unsigned int size;
    /* 0x0C */ MemControlBlock* top;
    /* 0x10 */ MemControlBlock* end;
    /* 0x14 */ MemControlBlock* nextFree;
} Heap; // size = 0x18

// Debug name ("file:line") for an allocation
typedef struct MemDebugInfo {
    /* 0x00 */ char name[0x20];
} MemDebugInfo; // size = 0x20

void func_0011BB08(void* ptr, char* file, int line);
void func_0011CA28(void*, int);
void* func_0011C358(int arg0, int arg1, int arg2, int arg3, char* arg4);
void* func_0011C6E0(int arg0, int arg1, void* ptr, unsigned int size);
void func_0011CA28(void*, s32);


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
