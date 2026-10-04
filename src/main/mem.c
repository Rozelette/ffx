#include "common.h"

typedef int long128 __attribute__ ((mode (TI)));
typedef unsigned int u_long128 __attribute__ ((mode (TI)));

extern char D_0054CAF4;
extern char D_0056E094;
extern char D_0056E095;
extern int D_0056E090;
extern s32 D_0056E040;
extern int D_00309F58;

typedef struct MemControlBlock_t {
    struct MemControlBlock_t* next;
    struct MemControlBlock_t* prev;
    union { // TODO bitset or int?
        struct {
            int b0 : 16;
            int b16 : 11;
            int b27 : 5;
        } base;
        unsigned int raw;
    } unk8;
    struct {
        unsigned int a : 4;
        unsigned int b : 22;
        unsigned int c : 1;
        unsigned int d : 2;
        unsigned int e : 3;
    } unkC;
} MemControlBlock;

extern MemControlBlock* D_0056E098;

typedef struct {
    char hasFreedNodes;
    char active;
    char unk2;
    char unk3;
    unsigned int id;
    unsigned int size;
    MemControlBlock* top;
    MemControlBlock* end;
    MemControlBlock* nextFree;
} Heap;

extern Heap D_0056E048[2];

typedef struct {
    char unk0;
    char pad1[0x1F];
} s5fbffe0;

// TODO better match
void func_0011B9E8(void)
{
    char* puVar1;
    int iVar2;

    if (D_0054CAF4 != '\0') {
        iVar2 = 0x7ff;
        puVar1 = (char*) 0x5fbffe0;
        do {
            *puVar1 = 0;
            iVar2 = iVar2 + -1;
            puVar1 = puVar1 + -0x20;
        } while (-1 < iVar2);
    }
    return;
}

void func_0011BA28(void* ptr) {
    MemControlBlock* mcb = ptr - sizeof(MemControlBlock);
    if (D_0054CAF4 != '\0' && ptr != NULL) {
        if (mcb->unk8.raw & 0x07FF0000) {
            ((s5fbffe0*)0x5fb0000)[(mcb->unk8.raw >> 16) & 0x7FF].unk0 = 0;
            mcb->unk8.raw = mcb->unk8.raw & 0xF800FFFF;
        }
    }
}

extern s5fbffe0 D_00550438;
s5fbffe0* func_0011BA80(void* ptr) {
    MemControlBlock* mcb = ptr - sizeof(MemControlBlock);
    s5fbffe0* s5fbffe0_array = (s5fbffe0*)0x5fb0000;

    if (D_0054CAF4 == '\0') {
        return &D_00550438;
    }

    if (ptr == NULL) {
        return &D_00550438;
    }

    if ((mcb->unk8.raw & 0x07FF0000) == 0) {
        return &D_00550438;
    }

    return &s5fbffe0_array[(mcb->unk8.raw >> 16) & 0x7FF];
}

INCLUDE_RODATA("asm/nonmatchings/main/mem", D_005503C0);

INCLUDE_RODATA("asm/nonmatchings/main/mem", D_005503C8);

INCLUDE_RODATA("asm/nonmatchings/main/mem", D_005503D0);

INCLUDE_RODATA("asm/nonmatchings/main/mem", D_005503D8);

INCLUDE_RODATA("asm/nonmatchings/main/mem", D_005503E0);

INCLUDE_RODATA("asm/nonmatchings/main/mem", D_005503E8);

INCLUDE_RODATA("asm/nonmatchings/main/mem", D_005503F0);

INCLUDE_RODATA("asm/nonmatchings/main/mem", D_005503F8);

INCLUDE_RODATA("asm/nonmatchings/main/mem", D_00550400);

INCLUDE_RODATA("asm/nonmatchings/main/mem", D_00550408);

INCLUDE_RODATA("asm/nonmatchings/main/mem", D_00550410);

INCLUDE_RODATA("asm/nonmatchings/main/mem", D_00550418);

INCLUDE_RODATA("asm/nonmatchings/main/mem", D_00550420);

INCLUDE_RODATA("asm/nonmatchings/main/mem", D_00550428);

INCLUDE_RODATA("asm/nonmatchings/main/mem", D_00550430);

INCLUDE_RODATA("asm/nonmatchings/main/mem", D_00550438);

INCLUDE_ASM("asm/nonmatchings/main/mem", func_0011BAD0);

static inline void getRa(void* ra) {
    asm (
        "move $15, %0\n\t"
        "sw   $31, 0($15)\n\t"
        : : "r"(ra) : "t7"
    );
}

void func_0011BFA8(void* ptr);
void func_0011BCD0(void* arg0) {
    MemControlBlock* temp_16 = arg0 - sizeof(MemControlBlock);

    if (arg0 != 0) {
        void* ra;

        getRa(&ra);
        func_0011BFA8(ra);
        temp_16->unkC.b = D_0056E090;
    }
}

void func_0011BD38(char arg0) {
    D_0056E094 = arg0;
}

void func_0011BD48(char arg0) {
    D_0056E095 = arg0;
}

void func_0011BD58(u32 arg0, u32 arg1, u32 arg2) {
    Heap* heap = &D_0056E048[arg0];
    MemControlBlock* end;
    MemControlBlock* temp_6_3;
    MemControlBlock* top;

    arg1 &= 0xFFFFFFFC;
    arg2 &= 0xFFFFFFFC;

    top = (MemControlBlock*)arg1;
    end = (MemControlBlock*)(arg2 - sizeof(MemControlBlock));
    heap->size = arg2 - arg1;
    heap->top = top;

    *(u_long128*)top = 0;

    heap->end = end;
    heap->id = arg0;

    top->next = end;
    top->prev = 0;
    top->unkC.a = 0;
    top->unkC.d = arg0;

    temp_6_3 = heap->end;

    *(u_long128*)temp_6_3 = 0;

    temp_6_3->prev = top;
    temp_6_3->next = NULL;
    temp_6_3->unkC.a = 0xF;
    temp_6_3->unkC.d = arg0;

    heap->active = 1;
    heap->hasFreedNodes = 0;
    heap->nextFree = heap->top;
}

void func_0011CD88(void*);
void func_0011BE10(void) {
    D_0056E040 = 0;
    D_0056E048[0].active = 0;
    D_0056E048[1].active = 0;
    func_0011BD38(1);
    func_0011BD48(0);
    if (D_0054CAF4 != 0) {
        func_0011BD58(0, 0xA00000, 0x01B00000);
        func_0011BD58(1, 0x03000000, 0x05D00000);
        func_0011B9E8();
    } else {
        func_0011BD58(0, 0xA00000, 0x01B00000);
    }
    func_0011CD88(D_0056E048[0].top);
}

// TODO ptr_diff
int func_0011BEA8(void* ptr) {
    MemControlBlock* mcb = ptr - sizeof(MemControlBlock);

    if (ptr == NULL) {
        return 0;
    }

    return (int)((char*)mcb->next - (char*)ptr);
}

void func_0011BEC0(MemControlBlock* mcb, int val) {
    s32 numQWords = ((u32)mcb->next - (u32)mcb - sizeof(MemControlBlock)) / 16;
    int* ptr = (int*)(mcb + 1);
    numQWords--;
    while (numQWords != -1) {
        ptr[3] = val;
        ptr[2] = val;
        ptr[1] = val;
        ptr[0] = val;
        ptr += 4;
        numQWords--;
    }
}

void func_0011BF08(int arg0) {
    D_00309F58 = arg0;
}

INCLUDE_ASM("asm/nonmatchings/main/mem", func_0011BF18);
/*
void func_0011BF18(Heap* heap) {
    MemControlBlock* curr;
    MemControlBlock* next;
    MemControlBlock* next2;
    for (curr = heap->top; curr->unkC.a != 0xF; curr = curr->next) {
        next = curr->next;
        if (curr->unkC.a == 0) {
            next2 = next;
            if (next->unkC.a == 0) {
                do {
                    int a3 = 0;
                    next2 = next2->next;
                    curr->next = next2;
                    next = next2;
                    if (*&a3) {
                        break;
                    }
                } while (next->unkC.a == 0);
            }
        }
        next->prev = curr;
    }

    heap->hasFreedNodes = 0;
}
*/

void func_0011BF98(void) {
    func_0011BF18(&D_0056E048[0]);
}

void func_0011BFA8(void* ptr) {
    D_0056E090 = (((int)ptr - 8) >> 2) & 0x7FFFFF;
}

int func_0011BFC8(MemControlBlock* mcb);
INCLUDE_ASM("asm/nonmatchings/main/mem", func_0011BFC8);

int func_0011C078(void* ptr) {
    Heap* heap = &D_0056E048[0];
    if (ptr == NULL) {
        return 0;
    } else if ((s32)ptr < (s32)heap->top) {
        return -10;
    } else if ((s32)ptr > (s32)heap->end) {
        return -20;
    } else {
        return func_0011BFC8((MemControlBlock*)(ptr - sizeof(MemControlBlock)));
    }
}

void func_0017B9F8(s32, char*, char*, ...);
void func_0011C0C8(MemControlBlock* mcb) {
    int err = func_0011BFC8(mcb);
    if (err != 0) {
        func_0017B9F8(0, "SG:MemCheckMCBerr", "the MCB broken maybe.code=%d\n", err);
    }
}

void func_0011CAD0(MemControlBlock* mcb, int arg1);
void func_0011CAF8(Heap*);
void func_0011C108(MemControlBlock* mcb, char* msg, int arg2)
{
    func_0011CAD0(mcb, 0);
    func_0011CAF8(&D_0056E048[mcb->unkC.d]);
}

u16 func_0011C150(MemControlBlock* mcb) {
    u16* data = (u16*)(mcb + 1);
    s32 length = ((u32)mcb->next - (u32)mcb - sizeof(MemControlBlock)) / sizeof(short);
    u16 ret = 0;
    for (length--; length != -1; data++, length--) {
        ret += *data;
    }

    return ret;
}

void func_001037F8(void);
void func_0011C1A0(Heap* heap) {
    MemControlBlock* curr = heap->top;
    if (heap->active) {
        while (curr != NULL) {
            if (curr->unkC.c) {
                u16 checksum = func_0011C150(curr);
                if ((curr->unk8.raw & 0xFFFF) != checksum) {
                    func_0011CAD0(curr, 0);
                    func_001037F8();
                    func_0011CAF8(&D_0056E048[curr->unkC.d]);
                    func_0011CAD0(curr, 0);
                    func_0017B9F8(0, "SgMemCheckHeap", "Memory CheckSum Error");
                }
            }

            if (curr->next != NULL) {
                if (func_0011BFC8(curr->next)) {
                    func_0011CAD0(curr, 0);
                    func_001037F8();
                    func_0011CAF8(&D_0056E048[curr->unkC.d]);
                    func_0011CAD0(curr, 0);
                    func_0017B9F8(0, "SgMemCheckHeap", "Memory Brocken");

                }
            }

            curr = curr->next;
        }
    }
}

void func_0011C2F0(int arg0) {
    func_0011C1A0(&D_0056E048[arg0]);
}

void func_0011C308(void) {
}

void func_0011C310(int arg0) {
    D_0056E040 = arg0 - 1;
}

INCLUDE_ASM("asm/nonmatchings/main/mem", func_0011C320);

INCLUDE_ASM("asm/nonmatchings/main/mem", func_0011C6A8);

void func_0011C7B0(void* arg0, int arg1) {
    MemControlBlock* temp_16 = arg0 - sizeof(MemControlBlock);
    temp_16->unkC.c = arg1;
    if (arg1) {
        temp_16->unk8.base.b0 = func_0011C150(temp_16);
    }
}

void func_0011C800(MemControlBlock* mcb);
INCLUDE_ASM("asm/nonmatchings/main/mem", func_0011C800);
/*
void func_0011C800(MemControlBlock* mcb) {
    Heap* heap = &D_0056E048[mcb->unkC.d];

    mcb->unk8.base.b16 = 0;
    mcb->unk8.base.b27 = 0;

    mcb->unkC.a = 0;
    mcb->unkC.b = 0;
    mcb->unkC.c = 0;


    mcb->unkC.e = 0;

    mcb->unk8.base.b0 = 0;

    if (mcb < heap->nextFree) {
        heap->nextFree = mcb;
    }
    heap->hasFreedNodes = 1;
}
*/

void func_0011C880(MemControlBlock* mcb);
INCLUDE_ASM("asm/nonmatchings/main/mem", func_0011C880);

void* func_002F1110(void *ptr,int value,int num); // memset
void func_0011C918(s32 arg0, MemControlBlock* arg1, MemControlBlock* arg2, MemControlBlock* arg3) {
    Heap* heap = &D_0056E048[arg0];

    func_002F1110(arg1, 0, sizeof(MemControlBlock));

    arg1->next = arg3;
    arg1->prev = arg2;
    arg1->unkC.d = arg0;

    arg2->next = arg1;
    arg1->next->prev = arg1;

    func_0011C800(arg1);
    func_0011C880(arg1);
    heap->hasFreedNodes = 0;
}

int func_0011C9D0(void* ptr) {
    MemControlBlock* temp_16 = ptr - sizeof(MemControlBlock);
    if (ptr == NULL) {
        return 0;
    }
    return temp_16->unkC.a == 0;
}

void func_0011C9F0(void* ptr, s32 arg1) {
    MemControlBlock* mcb = ptr - sizeof(MemControlBlock);
    if (ptr != NULL) {
        if (mcb->unkC.a == 0) {
            func_0011C108(mcb, "this ptr is already free.", 0);
        }

        if (mcb->unkC.a != arg1 && mcb->unkC.a != 0xE && arg1 != -1) {
            func_0011C108(mcb, "memory owner is differrent", 0);
        }

        func_0011BA28(ptr);
        func_0011C800(mcb);
    }
}

void func_0011F158(void*, int, char*);
void func_0011CAA8(void* ptr, char* arg1) {
    if (D_0054CAF4) {
        MemControlBlock* mcb = ptr - sizeof(MemControlBlock);
        func_0011F158(ptr, (int)((char*)mcb->next - (char*)ptr), arg1);
    }
}

void func_0011CAD0(MemControlBlock* mcb, int arg1) {
    if (mcb->unkC.a != 0 && mcb->unkC.a != 0xF) {
        func_0011BA80(mcb + 1);
    }
}

void func_0011CAF8(Heap* heap) {
    MemControlBlock* curr = heap->top;
    s32 s1;

    if (heap->hasFreedNodes) {
        func_0011BF18(heap);
    }

    for (s1 = 0; curr != NULL; s1++, curr = curr->next) {
        u32 size = (u32)curr->next - (u32)curr - sizeof(MemControlBlock);
        if ((size >= 0x40) || (curr->unkC.a != 0)) {
            func_0011CAD0(curr, s1);
        }
    }
}

void func_00110CB0(char*);
void func_0011CB80(s32 arg0) {
    switch (arg0) {
    case 1:
        func_0011CAF8(&D_0056E048[1]);
        break;
    case 2:
        func_0011CAF8(&D_0056E048[2]);
        break;
    default:
        func_0011CAF8(&D_0056E048[0]);
        break;
    }
    func_00110CB0("Cache Dump");
}

INCLUDE_ASM("asm/nonmatchings/main/mem", func_0011CBE0);

void* func_002F100C(void*, void*, s32); // memmove
INCLUDE_ASM("asm/nonmatchings/main/mem", func_0011CC40);
/*
void* func_0011CC40(void* ptr) {
    MemControlBlock* mcb = ptr - sizeof(MemControlBlock);
    MemControlBlock* oldNext = mcb->next;
    MemControlBlock* newMcb;
    u32 size;

    if (oldNext->unkC.a) {
        return ptr;
    }

    size = (u32)oldNext - (u32)mcb;
    newMcb = (u32)oldNext->next + sizeof(MemControlBlock) - size - sizeof(MemControlBlock);

    func_002F100C(newMcb, mcb, size);

    newMcb->next = oldNext;
    newMcb->prev = mcb;
    mcb->next = newMcb;
    oldNext->prev = newMcb;

    if (mcb->prev->unkC.a == 0) {
        mcb->prev->next = mcb->next;
    } else {
        func_0011C800(mcb);
    }

    return newMcb + 1;
}
*/

INCLUDE_ASM("asm/nonmatchings/main/mem", func_0011CCF8);
/*
void* func_0011CCF8(void* arg0, void* arg1) {
    MemControlBlock* mcb0 = arg0 - sizeof(MemControlBlock);
    MemControlBlock* mcb1 = arg1 - sizeof(MemControlBlock);
    u32 size0 = (u32)mcb0->next - (u32)mcb0;
    u32 size1 = (u32)mcb1->next - (u32)mcb1;
    MemControlBlock* ret = arg0 + (size0 - size1 - sizeof(MemControlBlock));

    func_002F100C(ret, mcb1, size1);
    func_0011C800(mcb1);

    ret->next = mcb0->next;
    mcb0->next->prev = ret;
    mcb0->next = ret;
    ret->prev = mcb0;

    return ret + 1;
}
*/

void func_0011CD88(void* ptr) {
    if (ptr != NULL) {
        D_0056E098 = ptr - sizeof(MemControlBlock);
    } else {
        D_0056E098 = D_0056E048[0].top;
    }
}

INCLUDE_ASM("asm/nonmatchings/main/mem", func_0011CDB0);

void func_0011CF80(void* arg0, int arg1) {
    MemControlBlock* temp_16 = arg0 - sizeof(MemControlBlock);
    temp_16->unk8.base.b27 = arg1;
}

void func_0011CFA8(void* arg0, int arg1) {
    MemControlBlock* temp_16 = arg0 - sizeof(MemControlBlock);
    temp_16->unkC.a = arg1 + 1;
}

INCLUDE_ASM("asm/nonmatchings/main/mem", func_0011CFD0);

int func_001035F8(void);
int func_00110D40(void);
unsigned int func_0011D140(int arg0) {
    MemControlBlock* temp_16 = D_0056E048[0].top;
    unsigned int ret = 0;

    if (arg0 == 1) {
        func_0011BF98();
    } else if (arg0 == 2) {
        while (func_001035F8());
        while (func_00110D40() == 0);
    }

    while (temp_16 != NULL) {
        if (temp_16->unkC.a == 0) {
            int temp2 = (unsigned int)temp_16->next - (unsigned int)temp_16 - sizeof(MemControlBlock);
            if (temp2 > ret) {
                ret = temp2;
            }
        }
        temp_16 = temp_16->next;
    }

    return ret;
}
