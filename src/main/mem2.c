
#include "common.h"
#include "mem.h"

// TODO fix the padding
static void foo() {
}

// This can't be in the header because this file assume it returns void* (or was implicitly called)
void* Mem_SaveRA(void*);
int func_002EEED0(const char* fmt, ...); // Printf

MemControlBlock* func_0011D230(s32 size, s32 alignPlus1, s32 alignLess1, MemDebugInfo* arg3, s32 line) {
    void* newBlock;
    MemControlBlock* ptr;
    void* ra;
    s32 tmpSize;
    s32 align;
    s32 type;
    s32 mask;

    align = alignPlus1 - 1;
    GetRa(&ra);
    type = alignLess1 + 1;
    Mem_SaveRA(ra);
    ptr = func_0011C358(0, 0, type, size, arg3->name);
    mask = (s32)ptr & align;
    if (mask != 0) {
        tmpSize = (align - mask) - 0xF;
        func_002EEED0("size=%x ptr=%x align=%x mask=%x tmpsize=%x ", size, ptr, align, mask, tmpSize);
        func_0011CA28(ptr, type);
        newBlock = func_0011C358(0, 0, type, tmpSize, NULL);
        //! @Bug this should take an argument.
        // TODO figure out what is in a0 when func_0011C358 returns.
        func_0011CBB8();
        ptr = func_0011C358(0, 0, type, size, arg3->name);
        func_0011CA28(newBlock, type);
        func_002EEED0("result=%x", ptr);
        func_002EEED0("\n");
    }
    func_0011BB08(ptr, arg3->name, line);
    return ptr;
}

MemControlBlock* func_0011D370(s32 size, s32 typeLess1, char* file, s32 line) {
    char str[0x400];
    void* ptr;
    void* ra;

    GetRa(&ra);
    Mem_SaveRA(ra);
    sprintf(str, "%s:%d", file != NULL ? file : "", line);
    ptr = func_0011C358(0, 0, typeLess1 + 1, size, str);
    func_0011BB08(ptr, file, line);
    return ptr;
}

extern s8 D_00565DF0;
MemControlBlock* func_0011D420(s32 size, s32 typeLess1, char* file, s32 line) {
    MemControlBlock* mcb;
    void* ra;
    if (D_00565DF0 == 0) {
        return NULL;
    }
    GetRa(&ra);
    Mem_SaveRA(ra);
    mcb = func_0011C358(0, 1, typeLess1 + 1, size, NULL);
    func_0011BB08(mcb, file, line);
    return mcb;
}

MemControlBlock* func_0011D4B8(s32 size, s32 type, char* file, s32 line) {
    char str[0x400];
    void* ptr;
    void* ra;

    GetRa(&ra);
    Mem_SaveRA(ra);
    sprintf(str, "%s:%d", file != NULL ? file : "", line);
    ptr = func_0011C358(1, 0, type + 1, size, str);
    func_0011BB08(ptr, file, line);
    return ptr;
}

void* func_0011D568(void* ptr, u32 size, s32 arg2) {
    return func_0011C6E0(0, arg2, ptr, size);
}

void func_0011D590(void* arg0, s32 arg1) {
    func_0011CA28(arg0, arg1 + 1);
}

MemControlBlock* func_0011D598(s32 arg0) {
    return func_0011D370(arg0, 6, "src/main/mem2.c", 0x6D);
}

void* func_0011D5C0(int len) {
    return func_0011D420(len, 6, "src/main/mem2.c", 0x71);
}

void func_0011D5E8(void* arg0) {
    func_0011D590(arg0, 6);
}

void* func_0011D5F0(void* ptr, u32 size) {
    return func_0011C6E0(1, 6, ptr, size);
}

void* func_0011D618(void* ptr, u32 size) {
    return func_0011C6E0(0, 6, ptr, size);
}

void* func_0011D640(char* arg0) {
    return func_002F03BC(func_0011D5C0(strlen(arg0) + 1), arg0);
}

extern void* sMem2SavedRA;

// malloc?
void* func_0011D678(s32 size) {
    GetRa(&sMem2SavedRA);

    Mem_SaveRA(sMem2SavedRA);
    return func_0011C358(0, 0, 0xE, size, NULL);
}

void func_0011D6C8(void* ptr) {
    func_0011CA28(ptr, 0xE);
}

void* func_0011D6D0(char* arg0) {
    return func_002F03BC(func_0011D370(strlen(arg0) + 1, 0xD, "src/main/mem2.c", 0x99), arg0);
}
