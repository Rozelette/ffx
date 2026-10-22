#include "common.h"
#include "mem.h"

#include "extern/elf.h"

extern s32 Sg_PcReadL(char* file, void* outBuf, s32 offset, s32 size);
extern int sceRead(int fd, void* buf, int nbyte);
extern int sceOpen(const char* filename, int flag, ... /* mode_t mode */);
extern int sceLseek(int fd, int offset, int whence);
extern void qsort(void* ptr, int count, int size, int (*comp)(const void*, const void*));

static int sFirstFuncIdx;
static int sLastFuncIdx;
static s8* sSymTbl;
static char* sStringTbl;
static u8 sHasFuncSyms;
static Elf32_Sym* sLastFoundFunc;

#define ALIGN_PTR(ptr)       \
    while ((ptr & 0xF) != 0) \
        ptr++;

// Sort symbols based on their address. Grouping local and global function separately from other symbols. Non-functions
// are pushed to the end. Function addresses are grouped in descending order.
s32 Elf_SymAddrComparator(const void* a, const void* b) {
    s32 aNotFunction;
    s32 bNotFunction;
    Elf32_Sym* aSym = (Elf32_Sym*)a;
    Elf32_Sym* bSym = (Elf32_Sym*)b;
    bNotFunction = 1;
    aNotFunction = 1;

    if (aSym->st_info == ELF32_ST_INFO(STB_LOCAL, STT_FUNC) || aSym->st_info == ELF32_ST_INFO(STB_GLOBAL, STT_FUNC))
        aNotFunction = 0;

    if (bSym->st_info == ELF32_ST_INFO(STB_LOCAL, STT_FUNC) || bSym->st_info == ELF32_ST_INFO(STB_GLOBAL, STT_FUNC))
        bNotFunction = 0;

    if (aNotFunction != bNotFunction) {
        return aNotFunction - bNotFunction;
    }
    return bSym->st_value - aSym->st_value;
}

// Original name Sg_AnalysysELFformat.
// Parses the symbol table for functions and stores a copy of the string table. Not usable in retail because the symbols
// (and their sections) are stripped.
void Elf_ParseSymbols(char* path) {
    Elf32_Ehdr hdr;
    Elf32_Phdr* programHeaders;
    intptr_t symPtr;
    s32 i;
    s32 i2;
    u8 symInfo;
    Elf32_Shdr* curHdr;
    Elf32_Shdr* sectionHeaders;
    Elf32_Shdr* strTblHdr;
    Elf32_Ehdr* hdrPtr;

    sHasFuncSyms = 0;
    symPtr = 0x05F01000;
    strTblHdr = NULL;
    sHasFuncSyms = 0;

    hdrPtr = &hdr;
    printf("Sg_AnalysysELFformat:%s\n", path);
    Sg_PcReadL(path, hdrPtr, 0, sizeof(Elf32_Ehdr));

    programHeaders = Mem_AllocDefault(hdrPtr->e_phentsize * hdrPtr->e_phnum);

    sectionHeaders = Mem_AllocDefault(hdrPtr->e_shentsize * hdrPtr->e_shnum);

    Sg_PcReadL(path, programHeaders, hdrPtr->e_phoff, hdrPtr->e_phentsize * hdrPtr->e_phnum);
    Sg_PcReadL(path, sectionHeaders, hdrPtr->e_shoff, hdrPtr->e_shentsize * hdrPtr->e_shnum);
    sSymTbl = NULL;
    sStringTbl = NULL;
    i = 0;
    if (hdrPtr->e_shnum != i) {
        do {
            if (sectionHeaders[i].sh_type == SHT_STRTAB)
                strTblHdr = &sectionHeaders[i];
        } while (++i < hdrPtr->e_shnum);
    }

    if (strTblHdr != NULL) {
        sStringTbl = symPtr;
        Sg_PcReadL(path, (void*)sStringTbl, strTblHdr->sh_offset, strTblHdr->sh_size);
        symPtr += strTblHdr->sh_size;
        ALIGN_PTR(symPtr)
    }
    i = 0;
    if (sStringTbl != NULL) {
        s32 numSyms = 0;
        for (; i < hdrPtr->e_shnum; i++) {
            curHdr = &sectionHeaders[i];
            if (curHdr->sh_type == SHT_SYMTAB) {
                numSyms = curHdr->sh_size / curHdr->sh_entsize;
                sSymTbl = (s8*)symPtr;
                Sg_PcReadL(path, (void*)sSymTbl, curHdr->sh_offset, curHdr->sh_size);
                symPtr += curHdr->sh_size;
                ALIGN_PTR(symPtr)
                break;
            }
        }
        printf("sg:sym_string=%x\n", sStringTbl);
        printf("sg:sym_table=%x\n", sSymTbl);
        printf("sg_debug_last=%x\n", (s8*)symPtr);
        if ((sSymTbl != NULL) && (sStringTbl != NULL)) {
            qsort(sSymTbl, numSyms, sizeof(Elf32_Sym), Elf_SymAddrComparator);
            sLastFuncIdx = 0;
            sFirstFuncIdx = 0;
            i = 0;

            for (; i < numSyms; i++) {
                symInfo = ((Elf32_Sym*)(sSymTbl))[i].st_info;
                if (symInfo != ELF32_ST_INFO(STB_LOCAL, STT_FUNC) && symInfo != ELF32_ST_INFO(STB_GLOBAL, STT_FUNC)) {
                    continue;
                }
                sFirstFuncIdx = i;
                break;
            }

            for (i2 = sFirstFuncIdx; i2 < numSyms; i2++) {
                if (((Elf32_Sym*)(sSymTbl))[i2].st_info == ELF32_ST_INFO(STB_LOCAL, STT_FUNC) ||
                    ((Elf32_Sym*)(sSymTbl))[i2].st_info == ELF32_ST_INFO(STB_GLOBAL, STT_FUNC)) {
                    sLastFuncIdx++;
                    continue;
                }
                break;
            }

            printf("top=%d nb=%d\n", sFirstFuncIdx, sLastFuncIdx);
            if (sLastFuncIdx != 0) {
                sHasFuncSyms = 1;
            }
        }
        Mem_FreeDefault(sectionHeaders);
        Mem_FreeDefault(programHeaders);
    }
}

// Original name Sg_GetFuncName.
// Get the name for a symbol using its vram address as the search key. Searches from the highest address down
// Value in this case is the vram address since its looking at an executable.
char* Elf_GetFuncNameFromAddr(u32 addr) {
    Elf32_Sym* sym;
    Elf32_Sym* firstSym;
    s32 i;

    sLastFoundFunc = NULL;
    if (sHasFuncSyms == 0) {
        return "";
    }

    firstSym = (Elf32_Sym*)sSymTbl;
    i = sLastFuncIdx;
    sym = &firstSym[sFirstFuncIdx];

    for (; --i != -1; sym++) {
        if (!(sym->st_value > addr)) {
            sLastFoundFunc = sym;
            return &sStringTbl[sym->st_name];
        }
    }
    return "";
}

// Original name Sg_GetLastFuncAddr
s32 Elf_GetLastFuncAddr(void) {
    if ((sLastFoundFunc == NULL) || (sHasFuncSyms == 0)) {
        return 0;
    }
    return sLastFoundFunc->st_value;
}

// Original name Sg_GetLastFuncSize.
s32 Elf_GetLastFuncSize(void) {
    if ((sLastFoundFunc == NULL) || (sHasFuncSyms == 0)) {
        return 0;
    }
    return sLastFoundFunc->st_size;
}
