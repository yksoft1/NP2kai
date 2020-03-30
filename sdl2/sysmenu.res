
enum {
	MID_STATIC		= 0,
	MID_POPUP,

	MID_RESET,
	MID_CONFIG,
	MID_NEWDISK,
	MID_CHANGEFONT,
	MID_EXIT,

	MID_SAVESTAT0,
	MID_SAVESTAT1,
	MID_SAVESTAT2,
	MID_SAVESTAT3,
	MID_SAVESTAT4,
	MID_SAVESTAT5,
	MID_SAVESTAT6,
	MID_SAVESTAT7,
	MID_SAVESTAT8,
	MID_SAVESTAT9,
	MID_LOADSTAT0,
	MID_LOADSTAT1,
	MID_LOADSTAT2,
	MID_LOADSTAT3,
	MID_LOADSTAT4,
	MID_LOADSTAT5,
	MID_LOADSTAT6,
	MID_LOADSTAT7,
	MID_LOADSTAT8,
	MID_LOADSTAT9,

	MID_FDD1OPEN,
	MID_FDD1EJECT,
	MID_FDD2OPEN,
	MID_FDD2EJECT,
	MID_FDD3OPEN,
	MID_FDD3EJECT,
	MID_FDD4OPEN,
	MID_FDD4EJECT,

	MID_SASI1OPEN,
	MID_SASI1EJECT,
	MID_SASI2OPEN,
	MID_SASI2EJECT,
	MID_IDE1OPEN,
	MID_IDE1EJECT,
	MID_IDE2OPEN,
	MID_IDE2EJECT,
	MID_IDECDOPEN,
	MID_IDECDEJECT,

	MID_SCSI0OPEN,
	MID_SCSI0EJECT,
	MID_SCSI1OPEN,
	MID_SCSI1EJECT,
	MID_SCSI2OPEN,
	MID_SCSI2EJECT,
	MID_SCSI3OPEN,
	MID_SCSI3EJECT,

	MID_WINDOW,
	MID_FULLSCREEN,
	MID_ROLNORMAL,
	MID_ROLLEFT,
	MID_ROLRIGHT,
	MID_DISPSYNC,
	MID_RASTER,
	MID_NOWAIT,
#if defined(SUPPORT_ASYNC_CPU)
	MID_ASYNCCPU,
#endif
	MID_AUTOFPS,
	MID_60FPS,
	MID_30FPS,
	MID_20FPS,
	MID_15FPS,
	MID_SCREENOPT,
#if defined(SUPPORT_WAB) && defined(SUPPORT_CL_GD5430)
	MID_WABOPT,
#endif

	MID_KEY,
	MID_JOY1,
	MID_JOY2,
	MID_MOUSEKEY,
	MID_XSHIFT,
	MID_XCTRL,
	MID_XGRPH,
	MID_XROLL,
	MID_SNDCAD,
	MID_BEEPOFF,
	MID_BEEPLOW,
	MID_BEEPMID,
	MID_BEEPHIGH,
	MID_NOSOUND,
	MID_PC9801_14,
	MID_PC9801_26K,
	MID_PC9801_86,
	MID_PC9801_26_86,
	MID_PC9801_86_CB,
	MID_PC9801_118,
	MID_PC9801_86_MX,
	MID_PC9801_86_118,
	MID_PC9801_MX,
	MID_SPEAKBOARD,
	MID_SPEAKBOARD86,
	MID_SPARKBOARD,
	MID_SOUNDORCHESTRA,
	MID_SOUNDORCHESTRAV,
	MID_LITTLEORCHESTRAL,
	MID_MMORCHESTRA,
#if defined(SUPPORT_SOUND_SB16)
	MID_SB16,
	MID_86_SB16,
	MID_MX_SB16,
	MID_118_SB16,
	MID_86MXSB16,
	MID_86118SB16,
#endif
	MID_AMD98,
	MID_WAVESTAR,
#if defined(SUPPORT_PX)
	MID_PX1,
	MID_PX2,
#endif
	MID_PC9801_118_ROM,
#if defined(SUPPORT_FMGEN)
	MID_FMGEN,
#endif
	MID_JASTSND,
	MID_SEEKSND,
	MID_MEM640,
	MID_MEM16,
	MID_MEM36,
	MID_MEM76,
	MID_MEM96,
	MID_MEM136,
	MID_MEM166,
	MID_MEM326,
	MID_MEM646,
	MID_MEM1206,
	MID_MEM2306,
	MID_MEM5126,
	MID_MEM10246,
	MID_SERIAL1,
	MID_MPUPC98,
	MID_MIDIPANIC,
	MID_KEYBOARD_106,
	MID_KEYBOARD_101,
	MID_KEY_COPY,
	MID_KEY_KANA,
	MID_KEY_YEN,
	MID_KEY_SYEN,
	MID_KEY_AT,
	MID_KEY_SAT,
	MID_KEY_UB,
	MID_KEY_SUB,
	MID_KEY_KPEQUALS,
	MID_KEY_KPCOMMA,
	MID_SNDOPT,

	MID_BMPSAVE,
	MID_S98LOGGING,
	MID_CALENDAR,
    MID_ALTENTER,
    MID_ALTF4,
	MID_DISPCLOCK,
	MID_DISPFRAME,
	MID_JOYX,
	MID_RAPID,
	MID_MSRAPID,
	MID_ITFWORK,
	MID_FIXMMTIMER,
	MID_WINNTIDEFIX,
	MID_SKIP16MBMEMCHK,
#if defined(SUPPORT_FAST_MEMORYCHECK)
	MID_FASTMEMCHK,
#endif
	MID_ABOUT
};

enum {
	MICON_NP2		= MICON_USER
};

