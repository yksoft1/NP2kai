/**
 * @file	scrnmng.cpp
 * @brief	Screen Manager (DirectDraw2)
 *
 * @author	$Author: yui $
 * @date	$Date: 2011/03/07 09:54:11 $
 */

#include "compiler.h"
#include <ddraw.h>
#ifndef __GNUC__
#include <winnls32.h>
#endif
#include "resource.h"
#include "np2.h"
#include "winloc.h"
#include "mousemng.h"
#include "scrnmng.h"
// #include "sysmng.h"
#include "dialog\np2class.h"
#include "pccore.h"
#include "scrndraw.h"
#include "palettes.h"

#if defined(SUPPORT_DCLOCK)
#include "subwnd\dclock.h"
#endif
#include "recvideo.h"

#ifdef SUPPORT_WAB
#include "wab/wab.h"
#endif

#include <shlwapi.h>

#if !defined(__GNUC__)
#pragma comment(lib, "ddraw.lib")
#pragma comment(lib, "dxguid.lib")
#endif	// !defined(__GNUC__)

//! 8BPP パレット数
#define PALLETES_8BPP	NP2PAL_TEXT3

extern WINLOCEX np2_winlocexallwin(HWND base);


typedef struct {
	LPDIRECTDRAW		ddraw1;
	LPDIRECTDRAW2		ddraw2;
	LPDIRECTDRAWSURFACE	primsurf;
	LPDIRECTDRAWSURFACE	backsurf;
#if defined(SUPPORT_DCLOCK)
	LPDIRECTDRAWSURFACE	clocksurf;
#endif
#if defined(SUPPORT_WAB)
	LPDIRECTDRAWSURFACE	wabsurf;
#endif
	LPDIRECTDRAWCLIPPER	clipper;
	LPDIRECTDRAWPALETTE	palette;
	UINT				scrnmode;
	int					width;
	int					height;
	int					extend;
	int					cliping;
	RGB32				pal16mask;
	UINT8				r16b;
	UINT8				l16r;
	UINT8				l16g;
	UINT8				menudisp;
	int					menusize;
	RECT				scrn;
	RECT				rect;
	RECT				scrnclip;
	RECT				rectclip;
	PALETTEENTRY		pal[256];
} DDRAW;

typedef struct {
	int		width;
	int		height;
	int		extend;
	int		multiple;
} SCRNSTAT;

static	DDRAW		ddraw;
		SCRNMNG		scrnmng;
static	SCRNSTAT	scrnstat;
static	SCRNSURF	scrnsurf;

#ifdef SUPPORT_WAB
int mt_wabdrawing = 0;
int mt_wabpausedrawing = 0;
#endif

static void setwindowsize(HWND hWnd, int width, int height)
{
	RECT workrc;
	SystemParametersInfo(SPI_GETWORKAREA, 0, &workrc, 0);
	const int scx = GetSystemMetrics(SM_CXSCREEN);
	const int scy = GetSystemMetrics(SM_CYSCREEN);

	// マルチモニタ暫定対応 ver0.86 rev30
	workrc.right = GetSystemMetrics(SM_CXVIRTUALSCREEN);
	workrc.bottom = GetSystemMetrics(SM_CYVIRTUALSCREEN);

	UINT cnt = 2;
	do
	{
		RECT rectwindow;
		GetWindowRect(hWnd, &rectwindow);
		RECT rectclient;
		GetClientRect(hWnd, &rectclient);
		int winx = (np2oscfg.winx != CW_USEDEFAULT) ? np2oscfg.winx : rectwindow.left;
		int winy = (np2oscfg.winy != CW_USEDEFAULT) ? np2oscfg.winy : rectwindow.top;
		int cx = width;
		cx += np2oscfg.paddingx * 2;
		cx += rectwindow.right - rectwindow.left;
		cx -= rectclient.right - rectclient.left;
		int cy = height;
		cy += np2oscfg.paddingy * 2;
		cy += rectwindow.bottom - rectwindow.top;
		cy -= rectclient.bottom - rectclient.top;

		if (scx < cx)
		{
			winx = (scx - cx) / 2;
		}
		else
		{
			if ((winx + cx) > workrc.right)
			{
				winx = workrc.right - cx;
			}
			if (winx < workrc.left)
			{
				winx = workrc.left;
			}
		}
		if (scy < cy)
		{
			winy = (scy - cy) / 2;
		}
		else
		{
			if ((winy + cy) > workrc.bottom)
			{
				winy = workrc.bottom - cy;
			}
			if (winy < workrc.top)
			{
				winy = workrc.top;
			}
		}
		MoveWindow(hWnd, winx, winy, cx, cy, TRUE);
	} while (--cnt);
}

static void renewalclientsize(BOOL winloc) {

	int			width;
	int			height;
	int			extend;
	UINT		fscrnmod;
	int			multiple;
	int			scrnwidth;
	int			scrnheight;
	int			tmpcy;
	WINLOCEX	wlex;

	width = np2min(scrnstat.width, ddraw.width);
	height = np2min(scrnstat.height, ddraw.height);

	extend = 0;

	// 描画範囲〜
	if (ddraw.scrnmode & SCRNMODE_FULLSCREEN) {
		ddraw.rect.right = width;
		ddraw.rect.bottom = height;
		scrnwidth = width;
		scrnheight = height;
		fscrnmod = np2oscfg.fscrnmod & FSCRNMOD_ASPECTMASK;
		switch(fscrnmod) {
			default:
			case FSCRNMOD_NORESIZE:
				break;

			case FSCRNMOD_ASPECTFIX8:
				scrnwidth = (ddraw.width << 3) / width;
				scrnheight = (ddraw.height << 3) / height;
				multiple = np2min(scrnwidth, scrnheight);
				scrnwidth = (width * multiple) >> 3;
				scrnheight = (height * multiple) >> 3;
				break;

			case FSCRNMOD_ASPECTFIX:
				scrnwidth = ddraw.width;
				scrnheight = (scrnwidth * height) / width;
				if (scrnheight >= ddraw.height) {
					scrnheight = ddraw.height;
					scrnwidth = (scrnheight * width) / height;
				}
				break;
				
			case FSCRNMOD_INTMULTIPLE:
				scrnwidth = (ddraw.width / width) * width;
				scrnheight = (scrnwidth * height) / width;
				if (scrnheight >= ddraw.height) {
					scrnheight = (ddraw.height / height) * height;
					scrnwidth = (scrnheight * width) / height;
				}
				break;
				
			case FSCRNMOD_FORCE43:
				if(ddraw.width*3 > ddraw.height*4){
					scrnwidth = ddraw.height*4/3;
					scrnheight = ddraw.height;
				}else{
					scrnwidth = ddraw.width;
					scrnheight = ddraw.width*3/4;
				}
				break;

			case FSCRNMOD_LARGE:
				scrnwidth = ddraw.width;
				scrnheight = ddraw.height;
				break;
		}
		ddraw.scrn.left = (ddraw.width - scrnwidth) / 2;
		ddraw.scrn.top = (ddraw.height - scrnheight) / 2;
		ddraw.scrn.right = ddraw.scrn.left + scrnwidth;
		ddraw.scrn.bottom = ddraw.scrn.top + scrnheight;

		// メニュー表示時の描画領域
		ddraw.rectclip = ddraw.rect;
		ddraw.scrnclip = ddraw.scrn;
		if (ddraw.scrnclip.top < ddraw.menusize) {
			ddraw.scrnclip.top = ddraw.menusize;
			tmpcy = ddraw.height - ddraw.menusize;
			if (scrnheight > tmpcy) {
				switch(fscrnmod) {
					default:
					case FSCRNMOD_NORESIZE:
						tmpcy = np2min(tmpcy, height);
						ddraw.rectclip.bottom = tmpcy;
						break;

					case FSCRNMOD_ASPECTFIX8:
					case FSCRNMOD_ASPECTFIX:
					case FSCRNMOD_INTMULTIPLE:
					case FSCRNMOD_FORCE43:
						ddraw.rectclip.bottom = (tmpcy * height) / scrnheight;
						break;
						
					case FSCRNMOD_LARGE:
						break;
				}
			}
			ddraw.scrnclip.bottom = ddraw.menusize + tmpcy;
		}
	}
	else {
		fscrnmod = np2oscfg.fscrnmod & FSCRNMOD_ASPECTMASK;
		multiple = scrnstat.multiple;
		if (!(ddraw.scrnmode & SCRNMODE_ROTATE)) {
			if ((np2oscfg.paddingx) && (multiple == 8)) {
				extend = np2min(scrnstat.extend, ddraw.extend);
			}
			scrnwidth = (width * multiple) >> 3;
			scrnheight = (height * multiple) >> 3;
			if(fscrnmod==FSCRNMOD_FORCE43) { // Force 4:3 Screen
				if(((width * multiple) >> 3)*3 < ((height * multiple) >> 3)*4){
					scrnwidth = ((height * multiple) >> 3)*4/3;
					scrnheight = ((height * multiple) >> 3);
				}else{
					scrnwidth = ((width * multiple) >> 3);
					scrnheight = ((width * multiple) >> 3)*3/4;
				}
			}
			ddraw.rect.right = width + extend;
			ddraw.rect.bottom = height;
			ddraw.scrn.left = np2oscfg.paddingx - extend;
			ddraw.scrn.top = np2oscfg.paddingy;
		}
		else {
			if ((np2oscfg.paddingy) && (multiple == 8)) {
				extend = np2min(scrnstat.extend, ddraw.extend);
			}
			scrnwidth = (height * multiple) >> 3;
			scrnheight = (width * multiple) >> 3;
			if(fscrnmod==FSCRNMOD_FORCE43) { // Force 4:3 Screen
				if(((width * multiple) >> 3)*4 < ((height * multiple) >> 3)*3){
					scrnwidth = ((height * multiple) >> 3)*3/4;
					scrnheight = ((height * multiple) >> 3);
				}else{
					scrnwidth = ((width * multiple) >> 3);
					scrnheight = ((width * multiple) >> 3)*4/3;
				}
			}
			ddraw.rect.right = height;
			ddraw.rect.bottom = width + extend;
			ddraw.scrn.left = np2oscfg.paddingx;
			ddraw.scrn.top = np2oscfg.paddingy - extend;
		}
		ddraw.scrn.right = np2oscfg.paddingx + scrnwidth;
		ddraw.scrn.bottom = np2oscfg.paddingy + scrnheight;

		wlex = NULL;
		if (winloc) {
			wlex = np2_winlocexallwin(g_hWndMain);
		}
		winlocex_setholdwnd(wlex, g_hWndMain);
		setwindowsize(g_hWndMain, scrnwidth, scrnheight);
		winlocex_move(wlex);
		winlocex_destroy(wlex);
	}
	scrnsurf.width = width;
	scrnsurf.height = height;
	scrnsurf.extend = extend;
}

static void clearoutofrect(const RECT *target, const RECT *base) {

	LPDIRECTDRAWSURFACE	primsurf;
	DDBLTFX				ddbf;
	RECT				rect;

	primsurf = ddraw.primsurf;
	if (primsurf == NULL) {
		return;
	}
	ZeroMemory(&ddbf, sizeof(ddbf));
	ddbf.dwSize = sizeof(ddbf);
	ddbf.dwFillColor = 0;

	rect.left = base->left;
	rect.right = base->right;
	rect.top = base->top;
	rect.bottom = target->top;
	if (rect.top < rect.bottom) {
		primsurf->Blt(&rect, NULL, NULL, DDBLT_COLORFILL, &ddbf);
	}
	rect.top = target->bottom;
	rect.bottom = base->bottom;
	if (rect.top < rect.bottom) {
		primsurf->Blt(&rect, NULL, NULL, DDBLT_COLORFILL, &ddbf);
	}

	rect.top = np2max(base->top, target->top);
	rect.bottom = np2min(base->bottom, target->bottom);
	if (rect.top < rect.bottom) {
		rect.left = base->left;
		rect.right = target->left;
		if (rect.left < rect.right) {
			primsurf->Blt(&rect, NULL, NULL, DDBLT_COLORFILL, &ddbf);
		}
		rect.left = target->right;
		rect.right = base->right;
		if (rect.left < rect.right) {
			primsurf->Blt(&rect, NULL, NULL, DDBLT_COLORFILL, &ddbf);
		}
	}
}

static void clearoutscreen(void) {

	RECT	base;
	POINT	clipt;
	RECT	target;

	GetClientRect(g_hWndMain, &base);
	clipt.x = 0;
	clipt.y = 0;
	ClientToScreen(g_hWndMain, &clipt);
	base.left += clipt.x;
	base.top += clipt.y;
	base.right += clipt.x;
	base.bottom += clipt.y;
	target.left = base.left + ddraw.scrn.left;
	target.top = base.top + ddraw.scrn.top;
	target.right = base.left + ddraw.scrn.right;
	target.bottom = base.top + ddraw.scrn.bottom;
	clearoutofrect(&target, &base);
}

static void clearoutfullscreen(void) {

	RECT	base;
const RECT	*scrn;

	base.left = 0;
	base.top = 0;
	base.right = ddraw.width;
	base.bottom = ddraw.height;
	if (GetWindowLongPtr(g_hWndMain, NP2GWLP_HMENU)) {
		scrn = &ddraw.scrn;
		base.top = 0;
	}
	else {
		scrn = &ddraw.scrnclip;
		base.top = ddraw.menusize;
	}
	clearoutofrect(scrn, &base);
#if defined(SUPPORT_DCLOCK)
	DispClock::GetInstance()->Redraw();
#endif
}

static void paletteinit()
{
	HDC hdc = GetDC(g_hWndMain);
	GetSystemPaletteEntries(hdc, 0, 256, ddraw.pal);
	ReleaseDC(g_hWndMain, hdc);
#if defined(SUPPORT_DCLOCK)
	const RGB32* pal32 = DispClock::GetInstance()->GetPalettes();
	for (UINT i = 0; i < 4; i++)
	 {
		ddraw.pal[i + START_PALORG].peBlue = pal32[i].p.b;
		ddraw.pal[i + START_PALORG].peRed = pal32[i].p.r;
		ddraw.pal[i + START_PALORG].peGreen = pal32[i].p.g;
		ddraw.pal[i + START_PALORG].peFlags = PC_RESERVED | PC_NOCOLLAPSE;
	}
#endif
	for (UINT i = 0; i < PALLETES_8BPP; i++)
	{
		ddraw.pal[i + START_PAL].peFlags = PC_RESERVED | PC_NOCOLLAPSE;
	}
	ddraw.ddraw2->CreatePalette(DDPCAPS_8BIT, ddraw.pal, &ddraw.palette, 0);
	ddraw.primsurf->SetPalette(ddraw.palette);
}

static void paletteset()
{
	if (ddraw.palette != NULL)
	{
		for (UINT i = 0; i < PALLETES_8BPP; i++)
		{
			ddraw.pal[i + START_PAL].peRed = np2_pal32[i].p.r;
			ddraw.pal[i + START_PAL].peBlue = np2_pal32[i].p.b;
			ddraw.pal[i + START_PAL].peGreen = np2_pal32[i].p.g;
		}
		ddraw.palette->SetEntries(0, START_PAL, PALLETES_8BPP, &ddraw.pal[START_PAL]);
	}
}

static void make16mask(DWORD bmask, DWORD rmask, DWORD gmask) {

	UINT8	sft;

	sft = 0;
	while((!(bmask & 0x80)) && (sft < 32)) {
		bmask <<= 1;
		sft++;
	}
	ddraw.pal16mask.p.b = (UINT8)bmask;
	ddraw.r16b = sft;

	sft = 0;
	while((rmask & 0xffffff00) && (sft < 32)) {
		rmask >>= 1;
		sft++;
	}
	ddraw.pal16mask.p.r = (UINT8)rmask;
	ddraw.l16r = sft;

	sft = 0;
	while((gmask & 0xffffff00) && (sft < 32)) {
		gmask >>= 1;
		sft++;
	}
	ddraw.pal16mask.p.g = (UINT8)gmask;
	ddraw.l16g = sft;
}

static void restoresurfaces() {
		ddraw.backsurf->Restore();
		ddraw.primsurf->Restore();
#if defined(SUPPORT_WAB)
		ddraw.wabsurf->Restore();
#endif
		scrndraw_updateallline();
}


// ----

void scrnmng_initialize(void) {

	scrnstat.width = 640;
	scrnstat.height = 400;
	scrnstat.extend = 1;
	scrnstat.multiple = 8;
	setwindowsize(g_hWndMain, 640, 400);
}

static TCHAR dd_displayName[32] = {0};
BOOL WINAPI DDEnumDisplayCallbackA(GUID FAR *lpGUID, LPSTR lpDriverDescription, LPSTR lpDriverName, LPVOID lpContext, HMONITOR hm) {
	MONITORINFOEX  monitorInfoEx;
	RECT rc;
	RECT rcwnd;
	if(hm){
		GetWindowRect(g_hWndMain, &rcwnd);
		monitorInfoEx.cbSize = sizeof(MONITORINFOEX);
		GetMonitorInfo(hm, &monitorInfoEx);
		_tcscpy(dd_displayName, monitorInfoEx.szDevice);
		if(IntersectRect(&rc, &monitorInfoEx.rcWork, &rcwnd)){
			*((LPGUID)lpContext) = *lpGUID;
			return FALSE;
		}
	}
	return TRUE;
}

BRESULT scrnmng_create(UINT8 scrnmode) {

	DWORD			winstyle;
	DWORD			winstyleex;
	LPDIRECTDRAW2	ddraw2;
	DDSURFACEDESC	ddsd;
	DDPIXELFORMAT	ddpf;
	int				width;
	int				height;
	UINT			bitcolor;
	UINT			fscrnmod;
	DEVMODE			devmode;
	GUID			devguid = {0};
	LPGUID			devlpguid;
	HRESULT			r;

	static UINT8 lastscrnmode = 0;
	static WINDOWPLACEMENT wp = {sizeof(WINDOWPLACEMENT)};

	ZeroMemory(&scrnmng, sizeof(scrnmng));
	winstyle = GetWindowLong(g_hWndMain, GWL_STYLE);
	winstyleex = GetWindowLong(g_hWndMain, GWL_EXSTYLE);
	if (scrnmode & SCRNMODE_FULLSCREEN) {
		//if(np2oscfg.mouse_nc){
		//	winstyle &= ~CS_DBLCLKS;
		//}else{
			winstyle |= CS_DBLCLKS;
		//}
		if(!(lastscrnmode & SCRNMODE_FULLSCREEN)){
			GetWindowPlacement(g_hWndMain, &wp);
		}
		scrnmode &= ~SCRNMODE_ROTATEMASK;
		scrnmng.flag = SCRNFLAG_FULLSCREEN;
		winstyle &= ~(WS_CAPTION | WS_SYSMENU | WS_THICKFRAME);
		winstyle |= WS_POPUP;
		winstyleex |= WS_EX_TOPMOST;
		ddraw.menudisp = 0;
		ddraw.menusize = GetSystemMetrics(SM_CYMENU);
		np2class_enablemenu(g_hWndMain, FALSE);
		SetWindowLong(g_hWndMain, GWL_STYLE, winstyle);
		SetWindowLong(g_hWndMain, GWL_EXSTYLE, winstyleex);
	}
	else {
		scrnmng.flag = SCRNFLAG_HAVEEXTEND;
		winstyle |= WS_SYSMENU;
		if(np2oscfg.mouse_nc){
			winstyle &= ~CS_DBLCLKS;
			if (np2oscfg.wintype != 0) {
				WINLOCEX	wlex;
				// XXX: メニューが出せなくなって詰むのを回避（暫定）
				np2oscfg.wintype = 0;
				np2oscfg.wintype = 0;
				wlex = np2_winlocexallwin(g_hWndMain);
				winlocex_setholdwnd(wlex, g_hWndMain);
				np2class_windowtype(g_hWndMain, np2oscfg.wintype);
				winlocex_move(wlex);
				winlocex_destroy(wlex);
			}
		}else{
			winstyle |= CS_DBLCLKS;
		}
		if (np2oscfg.thickframe) {
			winstyle |= WS_THICKFRAME;
		}
		if (np2oscfg.wintype < 2) {
			winstyle |= WS_CAPTION;
		}
		winstyle &= ~WS_POPUP;
		winstyleex &= ~WS_EX_TOPMOST;
		if(lastscrnmode & SCRNMODE_FULLSCREEN){
			char *strtmp;
			char szModulePath[MAX_PATH];
			GetModuleFileNameA(NULL, szModulePath, sizeof(szModulePath)/sizeof(szModulePath[0]));
			strtmp = strrchr(szModulePath, '\\');
			if(strtmp){
				*strtmp = 0;
			}else{
				szModulePath[0] = 0;
			}
			strcat(szModulePath, "\\ddraw.dll");
			if(PathFileExistsA(szModulePath) && !PathIsDirectoryA(szModulePath)){
				// DXGLが使われていそうなので素直なコードに変更
				SetWindowLong(g_hWndMain, GWL_STYLE, winstyle);
				SetWindowLong(g_hWndMain, GWL_EXSTYLE, winstyleex);
			}else{
				ShowWindow(g_hWndMain, SW_HIDE); // Aeroな環境でフルスクリーン→ウィンドウの時にシステムアイコンが消える対策のためのおまじない（その1）
				SetWindowLong(g_hWndMain, GWL_STYLE, winstyle);
				SetWindowLong(g_hWndMain, GWL_EXSTYLE, winstyleex);
				ShowWindow(g_hWndMain, SW_SHOWNORMAL); // Aeroな環境で(ry（その2）
				ShowWindow(g_hWndMain, SW_HIDE); // Aeroな環境で(ry（その3）
				ShowWindow(g_hWndMain, SW_SHOWNORMAL); // Aeroな環境で(ry（その4）
			}
			SetWindowPlacement(g_hWndMain, &wp);
		}else{
			SetWindowLong(g_hWndMain, GWL_STYLE, winstyle);
			SetWindowLong(g_hWndMain, GWL_EXSTYLE, winstyleex);
			GetWindowPlacement(g_hWndMain, &wp);
		}
	}
	
	if(np2oscfg.emuddraw){
		devlpguid = (LPGUID)DDCREATE_EMULATIONONLY;
	}else{
		devlpguid = NULL;
		if(scrnmode & SCRNMODE_FULLSCREEN){
			r = DirectDrawEnumerateExA(DDEnumDisplayCallbackA, &devguid, DDENUM_ATTACHEDSECONDARYDEVICES);
			if(devguid != GUID_NULL){
				devlpguid = &devguid;
			}
		}
	}
	if ((r = DirectDrawCreate(devlpguid, &ddraw.ddraw1, NULL)) != DD_OK) {
		// プライマリで再挑戦
		if (DirectDrawCreate(np2oscfg.emuddraw ? (LPGUID)DDCREATE_EMULATIONONLY : NULL, &ddraw.ddraw1, NULL) != DD_OK) {
			goto scre_err;
		}
	}
	ddraw.ddraw1->QueryInterface(IID_IDirectDraw2, (void **)&ddraw2);
	ddraw.ddraw2 = ddraw2;

	if (scrnmode & SCRNMODE_FULLSCREEN) {
#if defined(SUPPORT_DCLOCK)
		DispClock::GetInstance()->Initialize();
#endif
		ddraw2->SetCooperativeLevel(g_hWndMain,
										DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN | DDSCL_MULTITHREADED);
		width = np2oscfg.fscrn_cx;
		height = np2oscfg.fscrn_cy;
#ifdef SUPPORT_WAB
		if(!np2wabwnd.multiwindow && (np2wab.relay&0x3)){
			if(np2wab.realWidth>=640 && np2wab.realHeight>=400){
				width = np2wab.realWidth;
				height = np2wab.realHeight;
			}else{
				width = 640;
				height = 480;
			}
		}
#endif
		bitcolor = np2oscfg.fscrnbpp;
		fscrnmod = np2oscfg.fscrnmod;
		if ((fscrnmod & (FSCRNMOD_SAMERES | FSCRNMOD_SAMEBPP)) &&
			(dd_displayName[0] ? EnumDisplaySettings(dd_displayName, ENUM_REGISTRY_SETTINGS, &devmode) : EnumDisplaySettings(NULL, ENUM_REGISTRY_SETTINGS, &devmode))) {
			if (fscrnmod & FSCRNMOD_SAMERES) {
				width = devmode.dmPelsWidth;
				height = devmode.dmPelsHeight;
			}
			if (fscrnmod & FSCRNMOD_SAMEBPP) {
				bitcolor = devmode.dmBitsPerPel;
			}
		}
		if ((width == 0) || (height == 0)) {
			width = 640;
			height = (np2oscfg.force400)?400:480;
		}
		if (bitcolor == 0) {
#if !defined(SUPPORT_PC9821)
			bitcolor = (scrnmode & SCRNMODE_HIGHCOLOR)?16:8;
#else
			bitcolor = 16;
#endif
		}
		if (ddraw2->SetDisplayMode(width, height, bitcolor, 0, 0) != DD_OK) {
			width = 640;
			height = 480;
			if (ddraw2->SetDisplayMode(width, height, bitcolor, 0, 0) != DD_OK) {
				goto scre_err;
			}
		}
		ddraw2->CreateClipper(0, &ddraw.clipper, NULL);
		ddraw.clipper->SetHWnd(0, g_hWndMain);

		ZeroMemory(&ddsd, sizeof(ddsd));
		ddsd.dwSize = sizeof(ddsd);
		ddsd.dwFlags = DDSD_CAPS;
		ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
		if (ddraw2->CreateSurface(&ddsd, &ddraw.primsurf, NULL) != DD_OK) {
			goto scre_err;
		}
//		fullscrn_clearblank();

		ZeroMemory(&ddpf, sizeof(ddpf));
		ddpf.dwSize = sizeof(DDPIXELFORMAT);
		if (ddraw.primsurf->GetPixelFormat(&ddpf) != DD_OK) {
			goto scre_err;
		}

		ZeroMemory(&ddsd, sizeof(ddsd));
		ddsd.dwSize = sizeof(ddsd);
		ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
		ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
#ifdef SUPPORT_WAB
		if(!np2wabwnd.multiwindow){
			if(np2oscfg.fscrnmod & FSCRNMOD_SAMERES){
				int maxx = GetSystemMetrics(SM_CXSCREEN);
				int maxy = GetSystemMetrics(SM_CYSCREEN);
				ddsd.dwWidth = (1280 > maxx ? maxx : 1280);
				ddsd.dwHeight = (1024 > maxy ? maxy : 1024);
			}else{
				if((np2wab.relay&0x3)!=0 && np2wab.realWidth>=640 && np2wab.realHeight>=400){
					// 実サイズに
					ddsd.dwWidth = np2wab.realWidth;
					ddsd.dwHeight = np2wab.realHeight;
				}else{
					ddsd.dwWidth = 640;
					ddsd.dwHeight = 480;
				}
			}
		}else{
			ddsd.dwWidth = 640;
			ddsd.dwHeight = 480;
		}
#else
		ddsd.dwWidth = 640;
		ddsd.dwHeight = 480;
#endif
		if (ddraw2->CreateSurface(&ddsd, &ddraw.backsurf, NULL) != DD_OK) {
			goto scre_err;
		}
#ifdef SUPPORT_WAB
		if (ddraw2->CreateSurface(&ddsd, &ddraw.wabsurf, NULL) != DD_OK) {
			goto scre_err;
		}
#endif
		if (bitcolor == 8) {
			paletteinit();
		}
		else if (bitcolor == 16) {
			make16mask(ddpf.dwBBitMask, ddpf.dwRBitMask, ddpf.dwGBitMask);
		}
		else if (bitcolor == 24) {
		}
		else if (bitcolor == 32) {
		}
		else {
			goto scre_err;
		}
#if defined(SUPPORT_DCLOCK)
		DispClock::GetInstance()->SetPalettes(bitcolor);
		ZeroMemory(&ddsd, sizeof(ddsd));
		ddsd.dwSize = sizeof(ddsd);
		ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
		ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
		ddsd.dwWidth = DCLOCK_WIDTH;
		ddsd.dwHeight = DCLOCK_HEIGHT;
		ddraw2->CreateSurface(&ddsd, &ddraw.clocksurf, NULL);
		DispClock::GetInstance()->Reset();
#endif
	}
	else {
		ddraw2->SetCooperativeLevel(g_hWndMain, DDSCL_NORMAL | DDSCL_MULTITHREADED);

		ZeroMemory(&ddsd, sizeof(ddsd));
		ddsd.dwSize = sizeof(ddsd);
		ddsd.dwFlags = DDSD_CAPS;
		ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
		if (ddraw2->CreateSurface(&ddsd, &ddraw.primsurf, NULL) != DD_OK) {
			goto scre_err;
		}

		ddraw2->CreateClipper(0, &ddraw.clipper, NULL);
		ddraw.clipper->SetHWnd(0, g_hWndMain);
		ddraw.primsurf->SetClipper(ddraw.clipper);

		ZeroMemory(&ddpf, sizeof(ddpf));
		ddpf.dwSize = sizeof(DDPIXELFORMAT);
		if (ddraw.primsurf->GetPixelFormat(&ddpf) != DD_OK) {
			goto scre_err;
		}

		ZeroMemory(&ddsd, sizeof(ddsd));
		ddsd.dwSize = sizeof(ddsd);
		ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
		ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
#ifdef SUPPORT_WAB
		if(!np2wabwnd.multiwindow && (np2wab.relay&0x3)!=0 && np2wab.realWidth>=640 && np2wab.realHeight>=400){
			// 実サイズに
			width = ddsd.dwWidth = np2wab.realWidth;
			height = ddsd.dwHeight = np2wab.realHeight;
			ddsd.dwWidth++; // +1しないと駄目らしい
		}else{
			if (!(scrnmode & SCRNMODE_ROTATE)) {
				ddsd.dwWidth = 640 + 1;
				ddsd.dwHeight = 480;
			}
			else {
				ddsd.dwWidth = 480;
				ddsd.dwHeight = 640 + 1;
			}
			width = 640;
			height = 480;
		}
#else
		if (!(scrnmode & SCRNMODE_ROTATE)) {
			ddsd.dwWidth = 640 + 1;
			ddsd.dwHeight = 480;
		}
		else {
			ddsd.dwWidth = 480;
			ddsd.dwHeight = 640 + 1;
		}
		width = 640;
		height = 480;
#endif

		if (ddraw2->CreateSurface(&ddsd, &ddraw.backsurf, NULL) != DD_OK) {
			goto scre_err;
		}
#ifdef SUPPORT_WAB
		if (ddraw2->CreateSurface(&ddsd, &ddraw.wabsurf, NULL) != DD_OK) {
			goto scre_err;
		}
#endif
		bitcolor = ddpf.dwRGBBitCount;
		if (bitcolor == 8) {
			paletteinit();
		}
		else if (bitcolor == 16) {
			make16mask(ddpf.dwBBitMask, ddpf.dwRBitMask, ddpf.dwGBitMask);
		}
		else if (bitcolor == 24) {
		}
		else if (bitcolor == 32) {
		}
		else {
			goto scre_err;
		}
		ddraw.extend = 1;
	}
	scrnmng.bpp = (UINT8)bitcolor;
	scrnsurf.bpp = bitcolor;
	ddraw.scrnmode = scrnmode;
	ddraw.width = width;
	ddraw.height = height;
	ddraw.cliping = 0;
	renewalclientsize(TRUE); // XXX: スナップ解除等が起こるので暫定TRUE
	lastscrnmode = scrnmode;
//	screenupdate = 3;					// update!
#if defined(SUPPORT_WAB)
	mt_wabpausedrawing = 0; // MultiThread対策
#endif
	return(SUCCESS);

scre_err:
	scrnmng_destroy();
	return(FAILURE);
}

void scrnmng_destroy(void) {

	if (scrnmng.flag & SCRNFLAG_FULLSCREEN) {
		np2class_enablemenu(g_hWndMain, (!np2oscfg.wintype));
	}
#if defined(SUPPORT_DCLOCK)
	if (ddraw.clocksurf) {
		ddraw.clocksurf->Release();
		ddraw.clocksurf = NULL;
	}
#endif
#if defined(SUPPORT_WAB)
	if (ddraw.wabsurf) {
		mt_wabpausedrawing = 1; // MultiThread対策
		while(mt_wabdrawing) 
			Sleep(10);
		ddraw.wabsurf->Release();
		ddraw.wabsurf = NULL;
	}
#endif
	if (ddraw.backsurf) {
		ddraw.backsurf->Release();
		ddraw.backsurf = NULL;
	}
	if (ddraw.palette) {
		ddraw.palette->Release();
		ddraw.palette = NULL;
	}
	if (ddraw.clipper) {
		ddraw.clipper->Release();
		ddraw.clipper = NULL;
	}
	if (ddraw.primsurf) {
		ddraw.primsurf->Release();
		ddraw.primsurf = NULL;
	}
	if (ddraw.ddraw2) {
		if (ddraw.scrnmode & SCRNMODE_FULLSCREEN) {
			ddraw.ddraw2->SetCooperativeLevel(g_hWndMain, DDSCL_NORMAL);
		}
		ddraw.ddraw2->Release();
		ddraw.ddraw2 = NULL;
	}
	if (ddraw.ddraw1) {
		ddraw.ddraw1->Release();
		ddraw.ddraw1 = NULL;
	}
	ZeroMemory(&ddraw, sizeof(ddraw));
}

void scrnmng_querypalette(void) {

	if (ddraw.palette) {
		ddraw.primsurf->SetPalette(ddraw.palette);
	}
}

RGB16 scrnmng_makepal16(RGB32 pal32) {

	RGB32	pal;

	pal.d = pal32.d & ddraw.pal16mask.d;
	return((RGB16)((pal.p.g << ddraw.l16g) +
						(pal.p.r << ddraw.l16r) + (pal.p.b >> ddraw.r16b)));
}

void scrnmng_fullscrnmenu(int y) {

	UINT8	menudisp;

	if (scrnmng.flag & SCRNFLAG_FULLSCREEN) {
		menudisp = ((y >= 0) && (y < ddraw.menusize))?1:0;
		if (ddraw.menudisp != menudisp) {
			ddraw.menudisp = menudisp;
			if (menudisp == 1) {
				np2class_enablemenu(g_hWndMain, TRUE);
			}
			else {
				np2class_enablemenu(g_hWndMain, FALSE);
				clearoutfullscreen();
			}
		}
	}
}

void scrnmng_topwinui(void) {

	mousemng_disable(MOUSEPROC_WINUI);
	if (!ddraw.cliping++) {											// ver0.28
		if (scrnmng.flag & SCRNFLAG_FULLSCREEN) {
			ddraw.primsurf->SetClipper(ddraw.clipper);
		}
#ifndef __GNUC__
		WINNLSEnableIME(g_hWndMain, TRUE);
#endif
	}
}

void scrnmng_clearwinui(void) {

	if ((ddraw.cliping > 0) && (!(--ddraw.cliping))) {
#ifndef __GNUC__
		WINNLSEnableIME(g_hWndMain, FALSE);
#endif
		if (scrnmng.flag & SCRNFLAG_FULLSCREEN) {
			ddraw.primsurf->SetClipper(0);
		}
	}
	if (scrnmng.flag & SCRNFLAG_FULLSCREEN) {
		np2class_enablemenu(g_hWndMain, FALSE);
		clearoutfullscreen();
		ddraw.menudisp = 0;
	}
	else {
		if (np2oscfg.wintype) {
			np2class_enablemenu(g_hWndMain, FALSE);
			InvalidateRect(g_hWndMain, NULL, TRUE);
		}
	}
	mousemng_enable(MOUSEPROC_WINUI);
}

void scrnmng_setwidth(int posx, int width) {

	scrnstat.width = width;
	renewalclientsize(TRUE);
}

void scrnmng_setextend(int extend) {

	scrnstat.extend = extend;
	scrnmng.allflash = TRUE;
	renewalclientsize(TRUE);
}

void scrnmng_setheight(int posy, int height) {

	scrnstat.height = height;
	renewalclientsize(TRUE);
}

const SCRNSURF *scrnmng_surflock(void) {

	DDSURFACEDESC	destscrn;
	HRESULT			r;

	ZeroMemory(&destscrn, sizeof(destscrn));
	destscrn.dwSize = sizeof(destscrn);
	if (ddraw.backsurf == NULL) {
		return(NULL);
	}
	r = ddraw.backsurf->Lock(NULL, &destscrn, DDLOCK_WAIT, NULL);
	if (r == DDERR_SURFACELOST) {
		restoresurfaces();
		return(NULL);
		//r = ddraw.backsurf->Lock(NULL, &destscrn, DDLOCK_WAIT, NULL);
	}
	if (r != DD_OK) {
//		TRACEOUT(("backsurf lock error: %d (%d)", r));
		return(NULL);
	}
	if (!(ddraw.scrnmode & SCRNMODE_ROTATE)) {
		scrnsurf.ptr = (UINT8 *)destscrn.lpSurface;
		scrnsurf.xalign = scrnsurf.bpp >> 3;
		scrnsurf.yalign = destscrn.lPitch;
	}
	else if (!(ddraw.scrnmode & SCRNMODE_ROTATEDIR)) {
		scrnsurf.ptr = (UINT8 *)destscrn.lpSurface;
		scrnsurf.ptr += (scrnsurf.width + scrnsurf.extend - 1) * destscrn.lPitch;
		scrnsurf.xalign = 0 - destscrn.lPitch;
		scrnsurf.yalign = scrnsurf.bpp >> 3;
	}
	else {
		scrnsurf.ptr = (UINT8 *)destscrn.lpSurface;
		scrnsurf.ptr += (scrnsurf.height - 1) * (scrnsurf.bpp >> 3);
		scrnsurf.xalign = destscrn.lPitch;
		scrnsurf.yalign = 0 - (scrnsurf.bpp >> 3);
	}
	return(&scrnsurf);
}

void scrnmng_surfunlock(const SCRNSURF *surf) {

	ddraw.backsurf->Unlock(NULL);
	scrnmng_update();
	recvideo_update();
}

void scrnmng_update(void) {

	POINT	clip;
	RECT	dst;
	RECT	*rect;
	RECT	*scrn;
	HRESULT	r;

	if (scrnmng.palchanged) {
		scrnmng.palchanged = FALSE;
		paletteset();
	}
	if (ddraw.backsurf != NULL) {
		if (ddraw.scrnmode & SCRNMODE_FULLSCREEN) {
			if (scrnmng.allflash) {
				scrnmng.allflash = 0;
				clearoutfullscreen();
			}
			if (GetWindowLongPtr(g_hWndMain, NP2GWLP_HMENU)) {
				rect = &ddraw.rect;
				scrn = &ddraw.scrn;
			}
			else {
				rect = &ddraw.rectclip;
				scrn = &ddraw.scrnclip;
			}
			r = ddraw.primsurf->Blt(scrn, ddraw.backsurf, rect,
															DDBLT_WAIT, NULL);
			if (r == DDERR_SURFACELOST) {
				restoresurfaces();
				return;
				//ddraw.primsurf->Blt(scrn, ddraw.backsurf, rect,
				//											DDBLT_WAIT, NULL);
			}
		}
		else {
			if (scrnmng.allflash) {
				scrnmng.allflash = 0;
				clearoutscreen();
			}
			clip.x = 0;
			clip.y = 0;
			ClientToScreen(g_hWndMain, &clip);
			dst.left = clip.x + ddraw.scrn.left;
			dst.top = clip.y + ddraw.scrn.top;
			dst.right = clip.x + ddraw.scrn.right;
			dst.bottom = clip.y + ddraw.scrn.bottom;
			r = ddraw.primsurf->Blt(&dst, ddraw.backsurf, &ddraw.rect,
									DDBLT_WAIT, NULL);
			if (r == DDERR_SURFACELOST) {
				restoresurfaces();
				return;
				//ddraw.primsurf->Blt(&dst, ddraw.backsurf, &ddraw.rect,
				//										DDBLT_WAIT, NULL);
			}
		}
	}
}


// ----

void scrnmng_setmultiple(int multiple)
{
	if (scrnstat.multiple != multiple)
	{
		scrnstat.multiple = multiple;
		renewalclientsize(TRUE);
	}
}

int scrnmng_getmultiple(void)
{
	return scrnstat.multiple;
}



// ----

#if defined(SUPPORT_DCLOCK)
static const RECT rectclk = {0, 0, DCLOCK_WIDTH, DCLOCK_HEIGHT};

BOOL scrnmng_isdispclockclick(const POINT *pt) {

	if (pt->y >= (ddraw.height - DCLOCK_HEIGHT)) {
		return(TRUE);
	}
	else {
		return(FALSE);
	}
}

void scrnmng_dispclock(void)
{
	if (!ddraw.clocksurf)
	{
		return;
	}
	if (!DispClock::GetInstance()->IsDisplayed())
	{
		return;
	}

	const RECT* scrn;
	if (GetWindowLongPtr(g_hWndMain, NP2GWLP_HMENU))
	{
		scrn = &ddraw.scrn;
	}
	else
	{
		scrn = &ddraw.scrnclip;
	}
	if ((scrn->bottom + DCLOCK_HEIGHT) > ddraw.height)
	{
		return;
	}
	DispClock::GetInstance()->Make();

	DDSURFACEDESC dest;
	ZeroMemory(&dest, sizeof(dest));
	dest.dwSize = sizeof(dest);
	if (ddraw.clocksurf->Lock(NULL, &dest, DDLOCK_WAIT, NULL) == DD_OK)
	{
		DispClock::GetInstance()->Draw(scrnmng.bpp, dest.lpSurface, dest.lPitch);
		ddraw.clocksurf->Unlock(NULL);
	}
	if (ddraw.primsurf->BltFast(ddraw.width - DCLOCK_WIDTH - 4,
									ddraw.height - DCLOCK_HEIGHT,
									ddraw.clocksurf, (RECT *)&rectclk,
									DDBLTFAST_WAIT) == DDERR_SURFACELOST)
	{
		restoresurfaces();
		ddraw.clocksurf->Restore();
	}
	DispClock::GetInstance()->CountDown(np2oscfg.DRAW_SKIP);
}
#endif


// ----

typedef struct {
	int		bx;
	int		by;
	int		cx;
	int		cy;
	int		mul;
} SCRNSIZING;

static	SCRNSIZING	scrnsizing;

enum {
	SIZING_ADJUST	= 12
};

void scrnmng_entersizing(void) {

	RECT	rectwindow;
	RECT	rectclient;
	int		cx;
	int		cy;

	GetWindowRect(g_hWndMain, &rectwindow);
	GetClientRect(g_hWndMain, &rectclient);
	scrnsizing.bx = (np2oscfg.paddingx * 2) +
					(rectwindow.right - rectwindow.left) -
					(rectclient.right - rectclient.left);
	scrnsizing.by = (np2oscfg.paddingy * 2) +
					(rectwindow.bottom - rectwindow.top) -
					(rectclient.bottom - rectclient.top);
	cx = np2min(scrnstat.width, ddraw.width);
	cx = (cx + 7) >> 3;
	cy = np2min(scrnstat.height, ddraw.height);
	cy = (cy + 7) >> 3;
	if (!(ddraw.scrnmode & SCRNMODE_ROTATE)) {
		scrnsizing.cx = cx;
		scrnsizing.cy = cy;
	}
	else {
		scrnsizing.cx = cy;
		scrnsizing.cy = cx;
	}
	scrnsizing.mul = scrnstat.multiple;
}

void scrnmng_sizing(UINT side, RECT *rect) {

	int		width;
	int		height;
	int		mul;

	if ((side != WMSZ_TOP) && (side != WMSZ_BOTTOM)) {
		width = rect->right - rect->left - scrnsizing.bx + SIZING_ADJUST;
		width /= scrnsizing.cx;
	}
	else {
		width = 16;
	}
	if ((side != WMSZ_LEFT) && (side != WMSZ_RIGHT)) {
		height = rect->bottom - rect->top - scrnsizing.by + SIZING_ADJUST;
		height /= scrnsizing.cy;
	}
	else {
		height = 16;
	}
	mul = np2min(width, height);
	if (mul <= 0) {
		mul = 1;
	}
	else if (mul > 16) {
		mul = 16;
	}
	width = scrnsizing.bx + (scrnsizing.cx * mul);
	height = scrnsizing.by + (scrnsizing.cy * mul);
	switch(side) {
		case WMSZ_LEFT:
		case WMSZ_TOPLEFT:
		case WMSZ_BOTTOMLEFT:
			rect->left = rect->right - width;
			break;

		case WMSZ_RIGHT:
		case WMSZ_TOP:
		case WMSZ_TOPRIGHT:
		case WMSZ_BOTTOM:
		case WMSZ_BOTTOMRIGHT:
		default:
			rect->right = rect->left + width;
			break;
	}

	switch(side) {
		case WMSZ_TOP:
		case WMSZ_TOPLEFT:
		case WMSZ_TOPRIGHT:
			rect->top = rect->bottom - height;
			break;

		case WMSZ_LEFT:
		case WMSZ_RIGHT:
		case WMSZ_BOTTOM:
		case WMSZ_BOTTOMLEFT:
		case WMSZ_BOTTOMRIGHT:
		default:
			rect->bottom = rect->top + height;
			break;
	}
	scrnsizing.mul = mul;
}

void scrnmng_exitsizing(void)
{
	scrnmng_setmultiple(scrnsizing.mul);
	InvalidateRect(g_hWndMain, NULL, TRUE);		// ugh
}

// フルスクリーン解像度調整
void scrnmng_updatefsres(void) {
#ifdef SUPPORT_WAB
	int width = scrnstat.width;
	int height = scrnstat.height;

	if((np2oscfg.fscrnmod & FSCRNMOD_SAMERES) && (g_scrnmode & SCRNMODE_FULLSCREEN)){
		DDBLTFX ddbltfx = {0};
		ddbltfx.dwSize = sizeof(DDBLTFX);
		ddraw.primsurf->Blt(NULL,NULL,NULL,DDBLT_COLORFILL | DDBLT_WAIT, &ddbltfx);
		ddraw.backsurf->Blt(NULL,NULL,NULL,DDBLT_COLORFILL | DDBLT_WAIT, &ddbltfx);
		np2wab.lastWidth = 0;
		np2wab.lastHeight = 0;
		return;
	}
	if(scrnstat.width<100 || scrnstat.height<100) return;
	
	if(np2wab.lastWidth!=width || np2wab.lastHeight!=height){
		np2wab.lastWidth = width;
		np2wab.lastHeight = height;
		if((g_scrnmode & SCRNMODE_FULLSCREEN)!=0){
			g_scrnmode = g_scrnmode & ~SCRNMODE_FULLSCREEN;
			scrnmng_destroy();
			g_scrnmode = g_scrnmode | SCRNMODE_FULLSCREEN;
			if (scrnmng_create(g_scrnmode | SCRNMODE_FULLSCREEN) == SUCCESS) {
				g_scrnmode = g_scrnmode | SCRNMODE_FULLSCREEN;
			}
			else {
				if (scrnmng_create(g_scrnmode) != SUCCESS) {
					PostQuitMessage(0);
					return;
				}
			}
		}else if(ddraw.width < width || ddraw.height < height){
			scrnmng_destroy();
			if (scrnmng_create(g_scrnmode) != SUCCESS) {
				if (scrnmng_create(g_scrnmode | SCRNMODE_FULLSCREEN) != SUCCESS) { // フルスクリーンでリトライ
					PostQuitMessage(0);
					return;
				}
				g_scrnmode = g_scrnmode | SCRNMODE_FULLSCREEN;
			}
		}
		DDBLTFX ddbltfx = {0};
		ddbltfx.dwSize = sizeof(DDBLTFX);
		ddraw.primsurf->Blt(NULL,NULL,NULL,DDBLT_COLORFILL | DDBLT_WAIT, &ddbltfx);
		ddraw.backsurf->Blt(NULL,NULL,NULL,DDBLT_COLORFILL | DDBLT_WAIT, &ddbltfx);
	}
#endif
}

// ウィンドウアクセラレータ画面転送
void scrnmng_blthdc(HDC hdc) {
#if defined(SUPPORT_WAB)
	HRESULT	r;
	HDC hDCDD;
	mt_wabdrawing = 0;
	if (np2wabwnd.multiwindow) return;
	if (mt_wabpausedrawing) return;
	if (ddraw.wabsurf != NULL) {
		mt_wabdrawing = 1;
		r = ddraw.wabsurf->GetDC(&hDCDD);
		if (r == DD_OK){
			r = BitBlt(hDCDD, 0, 0, scrnstat.width, scrnstat.height, hdc, 0, 0, SRCCOPY);
			ddraw.wabsurf->ReleaseDC(hDCDD);
		}
		mt_wabdrawing = 0;
	}
#endif
}
void scrnmng_bltwab() {
#if defined(SUPPORT_WAB)
	RECT	*dst;
	RECT	src;
	RECT	dstmp;
	//DDBLTFX ddfx;
	HRESULT	r;
	int exmgn = 0;
	if (np2wabwnd.multiwindow) return;
	if (ddraw.backsurf != NULL) {
		if (ddraw.scrnmode & SCRNMODE_FULLSCREEN) {
			if (GetWindowLongPtr(g_hWndMain, NP2GWLP_HMENU)) {
				dst = &ddraw.rect;
			}
			else {
				dst = &ddraw.rectclip;
			}
		}else{
			dst = &ddraw.rect;
			exmgn = scrnstat.extend;
		}
		src.left = src.top = 0;
		src.right = scrnstat.width;
		src.bottom = scrnstat.height;
		dstmp = *dst;
		dstmp.left += exmgn;
		dstmp.right = dstmp.left + scrnstat.width;
		r = ddraw.backsurf->Blt(&dstmp, ddraw.wabsurf, &src, DDBLT_WAIT, NULL);
		if (r == DDERR_SURFACELOST) {
			restoresurfaces();
		}
	}
#endif
}