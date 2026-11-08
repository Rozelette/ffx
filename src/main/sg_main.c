// Game main loop (Sg_MainInit, Sg_MainLoop) and the global render/debug switches it reads.

#include "common.h"
#include <libgraph.h>
#include <libdma.h>
#include <libgp.h>
#include <devvif0.h>
#include <devvu0.h>
#include <devvu1.h>
#include <libpad.h>
#include <eekernel.h>
#include <eetypes.h>

#include <string.h>

#if defined(VERSION_US_ALPHA)
#define SG_ASM "asm/us_alpha/nonmatchings/dbg/arg"
#else
#define SG_ASM "asm/intl/nonmatchings/dbg/arg"
#endif

typedef struct Fuka {
    s32 ticks[14];
} Fuka;

// Elsewhere in sg
s32 _Sg_SetLRButtonMode_(s32 mode);
s32 _Sg_VramGetAddrZ_(void);
void _Sg_ControlMain_(void);
void _Sg_ControlPlayer_(void);
void _Sg_CameraSetPlayer_(void);
void _Sg_CalcCameraMatrix_(void);
void _Sg_SetScreenCenter_(s32 x, s32 y);
void _Sg_CalcScreenMatrix_(void);
void _Sg_EnableMouseCamera_(s32 enable);
void _Sg_DrawSetEnvironmentInit_(void);
void _Sg_DrawSetEnvironmentInitMovie_(void);
void _Sg_DrawSetEnvironment_(s32 page); // Never reads page
void _Sg_DrawFilter_(void);
void _Sg_DebugFill_(s32 color);
s32 _sg_AccSetCurrent_(void);
void Sg_AccSetFrameMap(void);
void Sg_AccSetFrameChr(void);
s32 Sg_AccDrawBG(void);
void _Sg_MFIFOStart_(void);
void _Sg_MFIFOEnd_(void);
void _Sg_MFIFOSpadSync_(void);
void _Sg_MFIFOSync_(void);
s32 Sg_GetIsPrerend(void);
s32 Sg_GetCurExecFrames(void);
void _Sg_SetExecSpecialSpeed_(s32 speed);
s32 _Sg_EnableTkAnti_(s32 enable);
void _Sg_SetTkAntiXYB_(s32 arg0, s32 arg1, s32 arg2);
void func_0011FC58(sceGsDispEnv* disp); // Writes PMODE/DISPFB/DISPLAY directly while TkAnti is on
void _Sg_GuiSetTextureAnim_(s32 enable);
void _Sg_InitAll_(void);
void _Sg_MapInitAll_(void);
void _Sg_IdInitAll_(void);
void _Sg_PlayerInitAll_(void);
s32 _Sg_IdDrawMain_(void);
s32 _Sg_DbDraw_(void);
s32 _Sg_Capture_(void);
s32 _Sg_CaptureGetGuiHide_(void);
s32 _Sg_PacketGetPtr_(void);
s32 func_00104518(s32 enable);
s32 func_001044D8(void);
s32 func_0021FC58(s32 menu);

// Ms (battle, camera and effects)
void _MsDebugInfoRead_(void);
void _MsBattleInit_(void);
void _MsBattleDraw_(void);
void _MsEffectInit_(void);
void MsEffectProcess(s32 mode);
void _MsScreenInit_(s32 width, s32 height);
void _MsCameraInit_(void);
void _MsCameraProcess_(void);
s32 _MsScreenProcess_(s32 page);
s32 _MsGetScreenData_(s32* arg0, s32* arg1, s32* arg2, s32* arg3, s32 arg4);
void _MsCopyCameraMatrix_(sceVu0FMATRIX dst);
void _MsCopyScreenMatrix_(sceVu0FMATRIX dst);

// Ch (characters)
void _Ch_InitAll_(void);
void _Ch_ChrMemHeapSet_(void);
void _Ch_CalcFast_(void);
void _Ch_CalcMain_(void);
void _Ch_CalcMiddle_Batch_(void);
void _Ch_MemMove_(void);
void _Ch_DrawMain_(void);
void _Ch_DrawShadow_(s32 arg0);
void _Ch_SysSetScreenMatrix_(sceVu0FMATRIX m);
void _Ch_SysSetCameraMatrix_(sceVu0FMATRIX m);

// yi (map)
void _yiInit_(void);
void _yiAnimMain_(void);
void _yiDisp_(void);
void _yiDisp2_(void);
void _yiDrawScrEffect_(void);
void _yiCallFieldParticle_(void);
void _yiSetScaleFactor_(sceVu0FVECTOR scale);
void _yiSetScreenMatrix_(sceVu0FMATRIX m);
void _yiSetCameraMatrix_(sceVu0FMATRIX m);
void _Yn_PreBgProc_(void);
void _Yn_PreBgDraw_(void);
void _Yn_PreBgDrawBeforeChr_(void);

// Tk, TO (event and message windows)
void _TkSg_IOPInit_(void);
void _TkInit_(void);
void _TkCtrl_(void);
void _TkDrawNoTrans_(void);
void _TkDrawSemiTrans_(void);
void _TkDrawDebug_(void);
void _TOControlMessageWindow_(void);
void _TODrawMessageWindow_(void);
void _TOSetEnvMovieMessage_(void);

// op, ppp (particles)
void _op_init_ffx_(void);
void _op_loop_ffx_before_(void);
void _op_loop_ffx_after_(void);
void _op_loop_end_ffx_(void);
s32 _op_gs1_transfer_(void);
void _op_ot_draw_(void);
void _op_num_vdisp_(void);
void _pppInitPartRender_(void);
void _pppGetPartMatrix_(sceVu0FMATRIX camera, sceVu0FMATRIX screen);
void _pppSetPartMatrix_(sceVu0FMATRIX camera, sceVu0FMATRIX screen);
void _pppPartLoop_(void);

// Other
s8 _FFXGetVideoMode_(void);
void _SsInitIopSound_(s32 arg0); // Never reads arg0
void _Cache_Init_(void);
void _movie_init_(void);
void _movie_disp_(void);
void _movie_disp2_(void);
s8 func_001577D8(void);
void func_001ACEB8(void);
void func_001C8B48(void);
void func_00111878(s32 arg0);
void func_00124E00(s32 arg0);
void func_002D35B0(void);
void func_002D3BC0(s32 arg0, void (*callback)(void));
s32 func_002D3C80(s32 arg0);

// Debug overlay (outside the main ELF)
void func_2031610(void);
void func_202C7B8(void);
void func_2031898(void);
s32 func_20318B8(sceDmaChan* chan, s32 packet);
s32 func_202A518(void);

extern void* Mem_StrDupDebug(char* arg0);
extern s8 gDebugMode;
extern s32 Sg_PaddOne;

// .data
char* _sg_user_name_ = "";

// clang-format off
// Width, height for each sg_disp_mode
s16 sScreenSizes[] = {
    512, 416,
    640, 448,
    320, 448,
    320, 224,
    640, 416,
    320, 416,
    512, 416,
};
// clang-format on

// Original name
s32 sg_count = 0;
// Incremented by Sg_VCountCallback
// Original name
s32 sg_vcount = 0;
// Original name
s32 sg_vcount2 = 0;

// Time scale for the current frame. 256 (1.0f) is one 30 fps frame
// Original name
s32 sg_rate = 0;
// Original name
f32 sg_ratef = 0.0f;
// Original name
s32 sg_screen_width = 0;
// Original name
s32 sg_screen_height = 0;
sceDmaChan* gDmaVIF0 = NULL;
sceDmaChan* gDmaVIF1 = NULL;
// Original name dmaGif
sceDmaChan* gDmaGIF = NULL;

// .bss
static sceGsDBuffDc sg_dbuff_dc;
static s8 sg_viewer_mode;
static s8 sg_effect_matrix;
static s8 sg_elf_type;
static s8 sg_disp_mode;
static s8 sg_disp_mode_bef;
static s8 sg_movie_gomi_avoid;
static s8 sg_calc_map;
static s8 sg_calc_chr;
// Update the camera on frames where the game logic is paused (0 exec frames).
// While the game is running the camera is updated every tick regardless of this flag.
static s8 sg_calc_cam;
static s8 sg_draw_map;
static s8 sg_draw_chr;
static s8 sg_draw_gui;
static s8 sg_draw_menu;
static s8 sg_effect_draw_page;
static s8 sg_map_alpha_fast;
static s8 sg_particle;
static s8 sg_effect_routine;
static s8 sg_th_disp;
static s8 sg_gui_draw_chr;
static s8 sg_pause;

// "Fuka" (load): EE timer 1 ticks spent in each part of the frame. ticks[0] is the whole frame.
// Sg_SetFuka(n) adds the ticks since the last call to ticks[n] and restarts the timer.
static Fuka sg_fuka_work;
static Fuka sg_fuka;
// Unused, but the .bss layout needs it
static s32 D_00580428;
static s32 sg_fuka_vu1;
static s32 sg_vcount_last;
static s8 sg_keep_fps;
static s32 sg_menu_reserve;
static s32 sg_menu_result;

// Original name
s32 Sg_GetElfType(void) {
    return sg_elf_type;
}

// Original name
void Sg_SetUserName(char* name) {
    _sg_user_name_ = Mem_StrDupDebug(name);
}

// Original name
s32 Sg_GetViewerMode(void) {
    return sg_viewer_mode;
}

// Original name
const char* Sg_GetUserName(void) {
    return _sg_user_name_;
}

// Original name
const char* Sg_GetUserName2(void) {
    return Sg_GetViewerMode() == 0 ? _sg_user_name_ : "sugimoto";
}

// Original name
void Sg_ArgInit(s32 argc, s8** argv) {
    _Sg_SetLRButtonMode_(0);
    sg_elf_type = -1;
    sg_effect_matrix = 0;
    sg_viewer_mode = 0;
    if (gDebugMode != 0) {
        if (argc >= 2) {
            Sg_SetUserName(argv[1]);
        }
    }
}

// Original name
sceGsDBuffDc* Sg_GetDBuffDC(s32* frameCount) {
    if (frameCount != NULL) {
        *frameCount = sg_count;
    }
    return &sg_dbuff_dc;
}

// Original name
s32 Sg_GetScreenWidth(void) {
    return sg_screen_width;
}

// Original name
s32 Sg_GetScreenHeight(void) {
    return sg_screen_height;
}

// Original name
s32 Sg_GetDispMode(void) {
    return sg_disp_mode;
}

// Original name
void Sg_SetScreenWH(s32 width, s32 height) {
    sg_screen_width = width;
    sg_screen_height = height;
}

// Original name
void Sg_SetDispMode(s32 mode) {
    sg_disp_mode = mode;
}

// Original name
void Sg_SetDispModeBef(s32 mode) {
    sg_disp_mode_bef = mode;
}

// Original name
void Sg_SetMovieGomiAvoid(s32 enable) {
    sg_movie_gomi_avoid = enable;
}

// Sets up display and draw environment `frame & 1` for the current sg_disp_mode.
// Buffer 0 starts at VRAM page 0 and buffer 1 directly after it.
// Original name
void Sg_SetDBuffDc(s32 frame) {
    s16 dy;
    s32 ztest;
    s32 zbp;
    s32 width;
    s32 height;
    s32 fbp0;
    s32 fbp1;

    dy = ((_FFXGetVideoMode_()) != 1) ? 0xE : 0x34;
    Sg_SetScreenWH(sScreenSizes[sg_disp_mode * 2], sScreenSizes[sg_disp_mode * 2 + 1]);

    ztest = (sg_disp_mode != 0) ? SCE_GS_ZNOUSE : SCE_GS_ZGREATER;

    width = sg_screen_width;
    height = sg_screen_height;

    // FBP and ZBP are 9-bit fields that count VRAM in 2048-word pages. A 24-bit color buffer still uses one
    // word per pixel, so the second buffer starts width * height / 2048 pages in.
    // Both arms are the same. The branch is needed to match (gcc stops treating fbp0 as a constant).
    if (sg_disp_mode) {
        fbp0 = 0;
        fbp1 = (sg_screen_width * sg_screen_height) / 2048;
    } else {
        fbp0 = 0;
        fbp1 = (sg_screen_width * sg_screen_height) / 2048;
    }

    if (frame & 1) {
        sceGsSetDefDispEnv(&sg_dbuff_dc.disp[0], SCE_GS_PSMCT24, width, height, 0, dy);
        sg_dbuff_dc.disp[0].dispfb.FBP = fbp0;
    } else {
        sceGsSetDefDispEnv(&sg_dbuff_dc.disp[1], SCE_GS_PSMCT24, width, height, 0, dy);
        sg_dbuff_dc.disp[1].dispfb.FBP = fbp1;
    }
    if ((frame & 1) == 0) {
        // A+D GIF tag (REGS0 = 0xE) for the 16 register writes after it: draw01's 8, then draw02's 8
        sg_dbuff_dc.giftag0.NLOOP = 0x10;
        sg_dbuff_dc.giftag0.EOP = 1;
        sg_dbuff_dc.giftag0.NREG = 1;
        sg_dbuff_dc.giftag0.PRE = 0;
        sg_dbuff_dc.giftag0.REGS0 = 0xE;
        // The GS has two drawing contexts. Both get the same settings.
        sceGsSetDefDrawEnv(&sg_dbuff_dc.draw01, SCE_GS_PSMCT24, width, height, ztest, SCE_GS_PSMZ24);
        sceGsSetDefDrawEnv2(&sg_dbuff_dc.draw02, SCE_GS_PSMCT24, width, height, ztest, SCE_GS_PSMZ24);
        sg_dbuff_dc.draw02.frame2.FBP = fbp1;
        sg_dbuff_dc.draw01.frame1.FBP = fbp1;
        zbp = (_Sg_VramGetAddrZ_() / 2048) & 0x1FF;
        sg_dbuff_dc.draw02.zbuf2.ZBP = zbp;
        sg_dbuff_dc.draw01.zbuf1.ZBP = zbp;
        return;
    }
    sg_dbuff_dc.giftag1.NLOOP = 0x10;
    sg_dbuff_dc.giftag1.EOP = 1;
    sg_dbuff_dc.giftag1.NREG = 1;
    sg_dbuff_dc.giftag1.PRE = 0;
    sg_dbuff_dc.giftag1.REGS0 = 0xE;
    sceGsSetDefDrawEnv(&sg_dbuff_dc.draw11, SCE_GS_PSMCT24, width, height, ztest, SCE_GS_PSMZ24);
    sceGsSetDefDrawEnv2(&sg_dbuff_dc.draw12, SCE_GS_PSMCT24, width, height, ztest, SCE_GS_PSMZ24);
    sg_dbuff_dc.draw12.frame2.FBP = 0;
    sg_dbuff_dc.draw11.frame1.FBP = 0;
    zbp = (_Sg_VramGetAddrZ_() / 2048) & 0x1FF;
    sg_dbuff_dc.draw12.zbuf2.ZBP = zbp;
    sg_dbuff_dc.draw11.zbuf1.ZBP = zbp;
}

// The Sg_Enable* setters return the previous value
// Original name
s32 Sg_EnableCalcMap(s32 enable) {
    s32 old = sg_calc_map;
    sg_calc_map = enable & 1;
    return old;
}

// Original name
s32 Sg_EnableGetCalcMap(void) {
    return sg_calc_map;
}

// Original name
s32 Sg_EnableCalcChr(s32 enable) {
    s32 old = sg_calc_chr;
    sg_calc_chr = enable & 1;
    return old;
}

// Original name
s32 Sg_EnableGetCalcChr(void) {
    return sg_calc_chr;
}

// Original name
s32 Sg_EnableCalcCam(s32 enable) {
    s32 old = sg_calc_cam;
    sg_calc_cam = enable & 1;
    return old;
}

// Original name
s32 Sg_EnableGetCalcCam(void) {
    return sg_calc_cam;
}

// Original name
s32 Sg_EnableDrawMap(s32 enable) {
    s32 old = sg_draw_map;
    sg_draw_map = enable & 1;
    return old;
}

// Original name
s32 Sg_EnableGetDrawMap(void) {
    return sg_draw_map;
}

// Original name
s32 Sg_EnableDrawChr(s32 enable) {
    s32 old = sg_draw_chr;
    sg_draw_chr = enable & 1;
    return old;
}

// Original name
s32 Sg_EnableGetDrawChr(void) {
    return sg_draw_chr;
}

// Original name
s32 Sg_EnableDrawGui(s32 enable) {
    s32 old = sg_draw_gui;
    sg_draw_gui = enable & 1;
    return old;
}

// Original name
s32 Sg_EnableGetDrawGui(void) {
    return sg_draw_gui;
}

// Original name
s32 Sg_EnableDrawMenu(s32 enable) {
    s32 old = sg_draw_menu;
    sg_draw_menu = enable & 1;
    return old;
}

// Original name
s32 Sg_EnableGetDrawMenu(void) {
    return sg_draw_menu;
}

// Empty in this build and on Switch. sg_effect_draw_page is set by Sg_MainLoop instead.
// Original name
void Sg_EffectSetDrawPage(s32 page) {
}

// Original name
s32 Sg_EffectGetDrawPage(void) {
    return sg_effect_draw_page;
}

// Original name
void Sg_SetMapAlphaFast(s32 enable) {
    sg_map_alpha_fast = enable;
}

// Original name
s32 Sg_GetMapAlphaFast(void) {
    return sg_map_alpha_fast;
}

// Original name
void Sg_SetthDispEnable(s32 enable) {
    sg_th_disp = enable & 1;
}

// Original name
s32 Sg_GetthDispEnable(void) {
    return sg_th_disp;
}

// Original name
void Sg_GuiSetDrawChr(s32 enable) {
    sg_gui_draw_chr = enable;
}

// Original name
s32 Sg_GuiGetDrawChr(void) {
    return sg_gui_draw_chr;
}

// Toggles sg_th_disp and returns 1 if it was on before
// Original name
s32 Sg_thDispEnable(void) {
    sg_th_disp = (sg_th_disp + 1) & 1;
    func_00104518(sg_th_disp);
    return 1 - sg_th_disp;
}

// Original name
void Sg_ToggleParticle(void) {
    sg_particle = (sg_particle + 1) & 1;
}

// Original name
s32 Sg_GetEnableParticle(void) {
    return sg_particle;
}

// Original name
s32 Sg_EnableParticle(s32 enable) {
    s32 old = sg_particle;
    sg_particle = enable & 1;
    return old;
}

// Original name
void Sg_EffectRoutineEnable(s32 enable) {
    sg_effect_routine = enable;
}

// Toggles. Doesn't take an argument
// Original name
void Sg_EnableEffectMatrix(void) {
    sg_effect_matrix = (sg_effect_matrix + 1) & 1;
}

// When set movement inputs are ignored. The game still appears to run
// Original name
void Sg_SetPause(s32 pause) {
    sg_pause = pause;
}

// Original name
s32 Sg_GetPause(void) {
    return sg_pause;
}

// Original name
void Sg_SetFukaFinish(void) {
    sg_fuka_work.ticks[0] += *T1_COUNT;
    *(&sg_fuka) = *(&sg_fuka_work);
}

// Original name
s32 Sg_FukaGet(void) {
    return sg_fuka.ticks[0];
}

// Original name
void Sg_SetFukaInit(void) {
    memset(&sg_fuka_work, 0, sizeof(sg_fuka_work));
    *T1_COUNT = 0;
}

// Original name
s32 Sg_GetFukaVu1(void) {
    return sg_fuka_vu1;
}

// Original name
s32 Sg_GetFuka(s32 index) {
    return sg_fuka.ticks[index];
}

// Original name
void Sg_SetFukaV(void) {
    sg_fuka_vu1 = sg_fuka_work.ticks[0] + *T1_COUNT;
}

// Original name
s32 Sg_SetFuka(s32 index) {
    s32 count;

    count = *T1_COUNT;
    sg_fuka_work.ticks[0] += count;

    if (index != -1) {
        sg_fuka_work.ticks[index] += count;
    }
    *T1_COUNT = 0;
    return 0;
}

// Returns the previous value
// Original name
s32 Sg_SetKeepFps(s32 keep) {
    s32 old = sg_keep_fps;
    sg_keep_fps = keep;
    return old;
}

// Original name
s32 Sg_GetKeepFps(void) {
    return sg_keep_fps;
}

// Original name
f32 Sg_GetKeepFpsRatef(void) {
    return sg_ratef;
}

// Original name
s32 Sg_GetKeepFpsRate(void) {
    return sg_rate;
}

#if defined(VERSION_US_ALPHA)
INCLUDE_ASM(SG_ASM, Sg_VCountCallback);
INCLUDE_ASM(SG_ASM, Sg_MainInit);
#else
// Registered with func_002D3BC0(1, ...) in Sg_MainInit. Not in the Switch build, which counts in Sg_MainLoop.
void Sg_VCountCallback(void) {
    sg_vcount += 1;
    sg_vcount2 += 1;
    func_002D35B0();
    if (gDebugMode != 0) {
        func_2031898();
    }
}

#define SG_SET_SCREEN_CENTER(x, y)      \
    do {                                \
        _Sg_SetScreenCenter_((x), (y)); \
        _Sg_CalcScreenMatrix_();        \
    } while (0)

void Sg_MenuSetReserve(s32 reserve);

// Original name
void Sg_MainInit(void) {
    s8 omode;

    // Reset VIF0, VU0, VU1 and the DMA controller
    sceDevVif0Reset();
    sceDevVu0Reset();
    sceDevVu1Reset();
    sceVpu0Reset();
    sceDmaReset(1);
    func_001ACEB8();
    _MsDebugInfoRead_();
    omode = (_FFXGetVideoMode_() != 1) ? SCE_GS_NTSC : SCE_GS_PAL;
    sceGsResetGraph(0, SCE_GS_INTERLACE, omode, SCE_GS_FIELD);
    sceGsResetPath();
    // DMA channels: GIF sends packets to the GS, VIF0 and VIF1 to VU0 and VU1
    gDmaGIF = sceDmaGetChan(SCE_DMA_GIF);
    gDmaVIF0 = sceDmaGetChan(SCE_DMA_VIF0);
    gDmaVIF1 = sceDmaGetChan(SCE_DMA_VIF1);
    sceGsSyncPath(0, 0U);
    Sg_SetDispMode(0);
    sceGsResetGraph(0, SCE_GS_INTERLACE, omode, SCE_GS_FIELD);
    Sg_SetDBuffDc(1);
    Sg_SetDBuffDc(0);
    FlushCache(0);
    sceGsSwapDBuffDc(&sg_dbuff_dc, 0);
    _Sg_DebugFill_(-1);
    _TkSg_IOPInit_();
    _SsInitIopSound_(0);
    _Cache_Init_();
    scePadInit(0);
    _movie_init_();
    _TkInit_();
    _Sg_InitAll_();
    _Ch_InitAll_();
    _Sg_MapInitAll_();
    _yiInit_();
    _MsScreenInit_(512, 416);
    _MsCameraInit_();
    if (gDebugMode != 0) {
        func_2031610();
        func_202C7B8();
    }
    _Sg_IdInitAll_();
    _Sg_PlayerInitAll_();
    _MsBattleInit_();
    _pppInitPartRender_();
    _op_init_ffx_();
    _op_loop_ffx_before_();
    _op_loop_ffx_after_();
    _MsEffectInit_();
    _Ch_ChrMemHeapSet_();
    _Sg_SetExecSpecialSpeed_(6);
    _Sg_EnableTkAnti_(1);
    Sg_EnableDrawMap(1);
    Sg_EnableDrawChr(1);
    Sg_EnableCalcMap(1);
    Sg_EnableCalcChr(1);
    Sg_EnableCalcCam(1);
    Sg_EnableDrawGui(1);
    Sg_EnableDrawMenu(1);
    _Sg_EnableMouseCamera_(1);
    Sg_EffectRoutineEnable(1);
    Sg_EnableParticle(1);
    Sg_EffectSetDrawPage(0);
    Sg_SetthDispEnable(1);
    Sg_GuiSetDrawChr(1);
    _Sg_GuiSetTextureAnim_(1);
    Sg_SetPause(0);
    Sg_MenuSetReserve(-1);
    func_002D3BC0(1, Sg_VCountCallback);
    Sg_SetMovieGomiAvoid(0);
    sg_vcount_last = sg_vcount;
    sg_count = 0;
    func_00124E00((gDebugMode != 0) ? 0 : 0x17);

    SG_SET_SCREEN_CENTER(256, 208);
}
#endif

// Original name
void Sg_MenuSetReserve(s32 reserve) {
    sg_menu_reserve = reserve;
    sg_menu_result = -1;
}

// Original name
s32 Sg_MenuGetResult(void) {
    return sg_menu_result;
}

#if defined(VERSION_US_ALPHA)
INCLUDE_ASM(SG_ASM, Sg_MainCheckMenu);
INCLUDE_ASM(SG_ASM, Sg_MainCalcRate);
INCLUDE_ASM(SG_ASM, Sg_MainDebug);
INCLUDE_ASM(SG_ASM, Sg_MainLoop);
#else
// Opens the menu requested with Sg_MenuSetReserve. Returns 1 when the menu ran, and Sg_MainLoop then
// skips the game logic and drawing for this frame.
// Original name
s32 Sg_MainCheckMenu(void) {
    s32 vcount;
    s32 result;
    s32 nextVcount;
    s32 ran;

    ran = 0;
    if (sg_menu_reserve != -1) {
        vcount = sg_vcount;
        result = func_0021FC58(sg_menu_reserve);
        sg_menu_result = result;
        if (result == 0) {
            nextVcount = vcount + 1;
            sg_vcount = nextVcount;
            ran = 1;
            sg_vcount_last = nextVcount;
        }
        sg_menu_reserve = -1;
    }
    if (Sg_PaddOne & 0x10) {
        Sg_MenuSetReserve(0);
    }
    return ran;
}

// sg_rate is 128 per vblank since the last frame, so 256 (1.0f) at 30 fps.
// Fixed at 256 unless exactly one exec frame runs this frame.
// Original name
void Sg_MainCalcRate(void) {
    u32 rate;

    rate = (u32)(sg_vcount - sg_vcount_last) * 128;
    sg_rate = rate;
    sg_ratef = rate / 256.0f;
    sg_vcount_last = sg_vcount;
    if (Sg_GetCurExecFrames() != 1) {
        sg_rate = 256;
        sg_ratef = 1.0f;
    }
}

#define SG_CAPTURE()                          \
    do {                                      \
        if (_Sg_Capture_())                   \
            _Sg_SetTkAntiXYB_(0, 1, 0x80);    \
    } while (0)

// Original name
void Sg_MainDebug(void) {
    _Sg_IdDrawMain_();
    Sg_SetFuka(4);
    if (gDebugMode != 0) {
        _Sg_DbDraw_();
        if ((_Sg_CaptureGetGuiHide_() == 0) && (sg_draw_gui != 0)) {
            func_20318B8(gDmaGIF, _Sg_PacketGetPtr_());
            func_001044D8();
        }
        func_202A518();
        SG_CAPTURE();
    }
}

// Original name
void Sg_MainLoop(void) {
    s32 ret;
    s32 screenData;
    s32 count;
    s32 pass;
    sceVu0FMATRIX screen;
    sceVu0FMATRIX camera;
    sceVu0FVECTOR scale;
    s32 i;

    scale[0] = scale[1] = scale[2] = 10.0f;
    _Sg_ControlMain_();
    if (Sg_MainCheckMenu() == 0) {
        Sg_MainCalcRate();
        Sg_SetFukaInit();
        _Ch_CalcFast_();

        // Game logic, once per exec frame
        for (i = 0; i < Sg_GetCurExecFrames(); i++) {
            _Sg_ControlPlayer_();
            func_001C8B48();
            _TkCtrl_();
            _TOControlMessageWindow_();
            Sg_SetFuka(5);
            if (sg_disp_mode == 0) {
                _op_loop_ffx_before_();
                MsEffectProcess(0);
                Sg_SetFuka(0xB);
            }
            _MsCameraProcess_();
            if (sg_disp_mode == 0) {
                _Yn_PreBgProc_();
                Sg_SetFuka(5);
            }
            if (sg_calc_chr != 0) {
                _Ch_CalcMain_();
            }
            Sg_SetFuka(2);
            _Sg_CameraSetPlayer_();
        }
        if (Sg_GetCurExecFrames() == 0) {
            MsEffectProcess(0);
            if (sg_calc_cam != 0) {
                _Sg_CameraSetPlayer_();
                _MsCameraProcess_();
            }
        }
        Sg_SetFuka(5);
        if (sg_calc_chr != 0) {
            _Ch_CalcMiddle_Batch_();
            Sg_SetFuka(2);
        }
        if (sg_draw_map != 0) {
            if (sg_th_disp != 0) {
                _yiAnimMain_();
                Sg_SetFuka(4);
            }
        }
        sg_disp_mode_bef = (u8)sg_disp_mode;
        _movie_disp_();
        Sg_SetFuka(7);
        Sg_SetDBuffDc(sg_count);
        if ((sg_disp_mode == 0) || (_Sg_DrawSetEnvironmentInitMovie_(), (sg_disp_mode == 0))) {
            _Sg_DrawSetEnvironmentInit_();
            _Yn_PreBgDrawBeforeChr_();
            _Sg_MFIFOStart_();
            _Sg_CalcCameraMatrix_();
            // Draw the map and characters for each screen page, in two passes
            for (pass = 0; pass < 2; pass++) {
                for (i = 0; (ret = _MsScreenProcess_(i)) <= 0; i++) {
                    if (ret != 0) {
                        continue;
                    }
                    screenData = _MsGetScreenData_(NULL, NULL, NULL, NULL, 0);
                    if ((((screenData != 0)) || (pass != 0)) && ((screenData != 1) || (pass == 0))) {
                        if (screenData == 0) {
                            sg_effect_draw_page = i;
                        }
                        if (sg_effect_matrix != 0) {
                            _pppGetPartMatrix_(camera, screen);
                        } else {
                            _MsCopyCameraMatrix_(camera);
                            _MsCopyScreenMatrix_(screen);
                            _pppSetPartMatrix_(camera, screen);
                        }
                        _yiSetScaleFactor_(scale);
                        _yiSetScreenMatrix_(screen);
                        _yiSetCameraMatrix_(camera);
                        _Ch_SysSetScreenMatrix_(screen);
                        _Ch_SysSetCameraMatrix_(camera);
                        _Sg_DrawSetEnvironment_(i);
                        Sg_SetFuka(3);
                        Sg_AccDrawBG();
                        if (sg_draw_map != 0) {
                            if (sg_th_disp != 0) {
                                Sg_AccSetFrameMap();
                                _yiDisp_();
                                Sg_SetFuka(4);
                            }
                        }
                        if (sg_map_alpha_fast != 0) {
                            if (sg_draw_map != 0) {
                                if (sg_th_disp != 0) {
                                    _yiDisp2_();
                                    Sg_SetFuka(4);
                                }
                            }
                            if (sg_draw_chr != 0) {
                                if (sg_gui_draw_chr != 0) {
                                    Sg_AccSetFrameChr();
                                    _Ch_DrawMain_();
                                    Sg_SetFuka(2);
                                }
                            }
                        } else {
                            if (sg_draw_chr != 0) {
                                if (sg_gui_draw_chr != 0) {
                                    Sg_AccSetFrameChr();
                                    _Ch_DrawMain_();
                                    Sg_SetFuka(2);
                                }
                            }
                            if ((sg_draw_map != 0) && (sg_th_disp != 0)) {
                                Sg_AccSetFrameMap();
                                _yiDisp2_();
                                Sg_SetFuka(4);
                            }
                        }
                    }
                }
            }
            _MsScreenProcess_(sg_effect_draw_page);
            _Sg_DrawSetEnvironment_(sg_effect_draw_page);
            _Sg_MFIFOEnd_();
            _Sg_MFIFOSpadSync_();
            _Sg_MFIFOSync_();
            Sg_SetFuka(8);

            // Effects and particles. Runs at least once even with 0 exec frames.
            if ((sg_effect_routine != 0) && (sg_disp_mode_bef == 0) && (sg_movie_gomi_avoid == 0)) {
                for (i = 0; i < ((count = Sg_GetCurExecFrames()) != 0 ? count : 1); i++) {
                    _op_loop_ffx_after_();
                    Sg_SetFuka(0xB);
                    if (i == 0) {
                        _MsBattleDraw_();
                        _TkDrawNoTrans_();
                        Sg_SetFuka(3);
                        if ((sg_draw_map != 0) && (sg_th_disp != 0)) {
                            _Yn_PreBgDraw_();
                            if (Sg_GetIsPrerend() != 0) {
                                _Ch_DrawShadow_(0);
                            }
                            Sg_SetFuka(4);
                        }
                    }
                    _op_gs1_transfer_();
                    if (sg_particle != 0) {
                        _yiCallFieldParticle_();
                    }
                    MsEffectProcess(1);
                    _pppPartLoop_();
                    Sg_SetFuka(0xB);
                    _Sg_MFIFOSync_();
                    Sg_SetFuka(8);
                    _op_ot_draw_();
                    Sg_SetFuka(0xC);
                }
            }
        }
        _Sg_MFIFOSync_();
        Sg_SetFuka(8);
        _movie_disp2_();
        Sg_SetFuka(7);
        _Sg_DrawFilter_();
        _sg_AccSetCurrent_();
        if (sg_disp_mode == 0) {
            if ((sg_draw_map != 0) && (sg_th_disp != 0)) {
                _yiDrawScrEffect_();
            }
            _TkDrawSemiTrans_();
            if (sg_draw_menu != 0) {
                _TODrawMessageWindow_();
            }
            _TkDrawDebug_();
            Sg_SetFuka(3);
            if ((sg_effect_routine != 0) && (gDebugMode != 0)) {
                _op_num_vdisp_();
            }
            Sg_SetFuka(0xB);
            Sg_MainDebug();
            Sg_SetFuka(1);
            _op_loop_end_ffx_();
            Sg_SetFuka(0xB);
        }
    }

    // End of frame: write the data cache back so DMA sees this frame's packets (FlushCache(0)), wait for
    // the GS paths to go idle (sceGsSyncPath), then swap buffers
    FlushCache(0);
    sceGsSyncPath(0, 0U);
    func_00111878(1);
    _Ch_MemMove_();
    Sg_SetFuka(3);
    Sg_SetFukaFinish();
    func_002D3C80(2);

    sg_count++;
    sceGsSwapDBuffDc(&sg_dbuff_dc, sg_count);
    _Sg_SetTkAntiXYB_(0, 1, 0x80);
    func_0011FC58(&sg_dbuff_dc.disp[sg_count & 1]);
    if (func_001577D8() != 0) {
        _TOSetEnvMovieMessage_();
    }
}

#endif
