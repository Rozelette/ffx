
#include "common.h"
#include "mem.h"

// TODO fix the padding
static void foo() {
}

// This can't be in the header because this file assume it returns void* (or was implicitly called)
void* func_0011BFE0(void*);
int func_002EEED0(const char* fmt, ...); // Printf

MemControlBlock* func_0011D230(s32 size, s32 arg1, s32 arg2, MemDebugInfo* arg3, s32 arg4) {
    void* newBlock;
    MemControlBlock* ptr;
    void* ra;
    s32 tmpSize;
    s32 align;
    s32 temp_19;
    s32 mask;

    align = arg1 - 1;
    GetRa(&ra);
    temp_19 = arg2 + 1;
    func_0011BFE0(ra);
    ptr = func_0011C358(0, 0, temp_19, size, arg3->name);
    mask = (s32)ptr & align;
    if (mask != 0) {
        tmpSize = (align - mask) - 0xF;
        func_002EEED0("size=%x ptr=%x align=%x mask=%x tmpsize=%x ", size, ptr, align, mask, tmpSize);
        func_0011CA28(ptr, temp_19);
        newBlock = func_0011C358(0, 0, temp_19, tmpSize, NULL);
        //! @Bug this should take an argument.
        // TODO figure out what is in a0 when func_0011C358 returns.
        func_0011CBB8();
        ptr = func_0011C358(0, 0, temp_19, size, arg3->name);
        func_0011CA28(newBlock, temp_19);
        func_002EEED0("result=%x", ptr);
        func_002EEED0("\n");
    }
    func_0011BB08(ptr, arg3->name, arg4);
    return ptr;
}

MemControlBlock* func_0011D370(s32 arg0, s32 arg1, char* arg2, s32 arg3) {
    char str[0x400];
    void* ptr;
    void* ra;

    GetRa(&ra);
    func_0011BFE0(ra);
    sprintf(str, "%s:%d", arg2 != NULL ? arg2 : "", arg3);
    ptr = func_0011C358(0, 0, arg1 + 1, arg0, str);
    func_0011BB08(ptr, arg2, arg3);
    return ptr;
}

extern s8 D_00565DF0;
MemControlBlock* func_0011D420(s32 arg0, s32 arg1, char* arg2, s32 arg3) {
    MemControlBlock* temp_2;
    void* ra;
    if (D_00565DF0 == 0) {
        return NULL;
    }
    GetRa(&ra);
    func_0011BFE0(ra);
    temp_2 = func_0011C358(0, 1, arg1 + 1, arg0, NULL);
    func_0011BB08(temp_2, arg2, arg3);
    return temp_2;
}

MemControlBlock* func_0011D4B8(s32 arg0, s32 arg1, char* arg2, s32 arg3) {
    char str[0x400];
    void* ptr;
    void* ra;

    GetRa(&ra);
    func_0011BFE0(ra);
    sprintf(str, "%s:%d", arg2 != NULL ? arg2 : "", arg3);
    ptr = func_0011C358(1, 0, arg1 + 1, arg0, str);
    func_0011BB08(ptr, arg2, arg3);
    return ptr;
}

void* func_0011D568(void* arg0, s32 arg1, s32 arg2) {
    return func_0011C6E0(NULL, arg2, arg0, arg1);
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

// void* func_0011C6E0(s32 arg0, s32 arg1, void* ptr, u32 size)

void* func_0011D5F0(void* ptr, u32 size) {
    return func_0011C6E0(1, 6, ptr, size);
}

void* func_0011D618(void* ptr, u32 size) {
    return func_0011C6E0(0, 6, ptr, size);
}

void* func_0011D640(char* arg0) {
    return func_002F03BC(func_0011D5C0(strlen(arg0) + 1), arg0);
}

extern void* D_0058049C;

void* func_0011D678(s32 arg0) {
    GetRa(&D_0058049C);

    func_0011BFE0(D_0058049C);
    return func_0011C358(0, 0, 0xE, arg0, NULL);
}

void func_0011D6C8(void* arg0) {
    func_0011CA28(arg0, 0xE);
}

void* func_0011D6D0(char* arg0) {
    return func_002F03BC(func_0011D370(strlen(arg0) + 1, 0xD, "src/main/mem2.c", 0x99), arg0);
}
