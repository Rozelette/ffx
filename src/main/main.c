#include "common.h"

int func_002E98A8(void);
int Mem_Init(void);
int _Sg_ArgInit_(int argc, char** argv);
int func_0011ADE0(void);
int func_0011B2B8(void);


// GCC automatically renames `main` to `__main` for external references and this can't be disabled.
int ffx_main(int argc, char** argv) __asm__("main");

int ffx_main(int argc, char** argv) {
    func_002E98A8();
    Mem_Init();
    _Sg_ArgInit_(argc, argv);
    func_0011ADE0();
    do {
        func_0011B2B8();
    } while (1);
}