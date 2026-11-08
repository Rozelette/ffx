#include "common.h"

int func_002EBC58(void);
int Mem_Init(void);
int _Sg_ArgInit_(int argc, char** argv);
int func_0011ADB0(void);
int func_0011B280(void);

int func_0011B9A0(int argc, char** argv) {
    func_002EBC58();
    Mem_Init();
    _Sg_ArgInit_(argc, argv);
    func_0011ADB0();
    do {
        func_0011B280();
    } while (1);
}