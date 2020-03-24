#define _POSIX_C_SOURCE 199309L
#include	"compiler.h"
#include	"inputmng.h"
#include	"taskmng.h"
#include	"kbtrans.h"
#include	"vramhdl.h"
#include	"menubase.h"
#include	"sysmenu.h"
#include	"scrnmng.h"
#include	"mousemng.h"
#include	"np2.h"

#if defined(__LIBRETRO__)
#include <retro_miscellaneous.h>
#include <retro_timers.h>
#include "libretro.h"

extern retro_environment_t environ_cb;
#endif	/* __LIBRETRO__ */

	BOOL	task_avail;


void sighandler(int signo) {

	(void)signo;
	task_avail = FALSE;
}


void taskmng_initialize(void) {

	task_avail = TRUE;
}

void taskmng_exit(void) {

	task_avail = FALSE;
}

#if defined(GCW0)

int ENABLE_MOUSE=0; //0--disable

#if SDL_MAJOR_VERSION != 1
int convertKeyMap(int scancode){
  switch(scancode){
    case 82: //up
      return SDL_SCANCODE_UP;
    case 81: //down
      return SDL_SCANCODE_DOWN;
    case 80: //left
      return SDL_SCANCODE_LEFT;
    case 79: //right
      return SDL_SCANCODE_RIGHT;
    case 225: //x
      return SDL_SCANCODE_UNKNOWN;
    case 44:  //y
      return SDL_SCANCODE_UNKNOWN;
    case 224: //A
      return  SDL_SCANCODE_RETURN;
    case 226: //B
      return SDLK_TAB;
    case 41: //select
      return SDL_SCANCODE_ESCAPE;
    case 40: //start
      return SDL_SCANCODE_RETURN;
    case 43: //L
      return SDL_SCANCODE_F11;
    case 42: //R
      // ENABLE_MOUSE++;
      // ENABLE_MOUSE=ENABLE_MOUSE%2;
      return 	999;
  }
}
#endif

#endif //GCW0

int mx = 320, my = 240;

void taskmng_rol(void) {

#if !defined(__LIBRETRO__)
	SDL_Event	e;


	if ((!task_avail) || (!SDL_PollEvent(&e))) {
		return;
	}
	switch(e.type) {
		case SDL_MOUSEMOTION:
			if (menuvram == NULL) {
				mousemng_onmove(&e.motion);
			} else {
				if((scrnmode & SCRNMODE_ROTATEMASK) == SCRNMODE_ROTATELEFT) {
					mx = (menuvram->width - 1) - e.motion.y;
					my = e.motion.x;
				} else if((scrnmode & SCRNMODE_ROTATEMASK) == SCRNMODE_ROTATERIGHT) {
					mx = e.motion.y;
					my = (menuvram->height - 1) - e.motion.x;
				} else {
					mx = e.motion.x;
					my = e.motion.y;
				}
				menubase_moving(mx, my, 0);
			}
			break;

		case SDL_MOUSEBUTTONUP:
			switch(e.button.button) {
				case SDL_BUTTON_LEFT:
					if (menuvram != NULL)
					{
//						menubase_moving(e.button.x, e.button.y, 2);
						menubase_moving(mx, my, 2);
					}
#if defined(__IPHONEOS__)
					else if (SDL_IsTextInputActive())
					{

						SDL_StopTextInput();
					}
					else if (e.button.y >= 320)
					{
						SDL_StartTextInput();
					}
#endif
					else
					{
						mousemng_buttonevent(&e.button);
					}
					break;

				case SDL_BUTTON_RIGHT:
					if (menuvram == NULL) {
						mousemng_buttonevent(&e.button);
					}
					break;
			}
			break;

		case SDL_MOUSEBUTTONDOWN:
			switch(e.button.button) {
				case SDL_BUTTON_LEFT:
					if (menuvram != NULL)
					{
//						menubase_moving(e.button.x, e.button.y, 1);
						menubase_moving(mx, my, 1);
					} else {
						mousemng_buttonevent(&e.button);
					}
					break;

				case SDL_BUTTON_RIGHT:
					if (menuvram == NULL) {
						mousemng_buttonevent(&e.button);
					}
					break;

				case SDL_BUTTON_MIDDLE:
					if (menuvram == NULL) {
//						sysmenu_menuopen(0, e.button.x, e.button.y);
						sysmenu_menuopen(0, mx, my);
					} else {
						menubase_close();
					}
					break;
			}
			break;

		case SDL_KEYDOWN:

#if defined(GCW0)
#if SDL_MAJOR_VERSION != 1
      e.key.keysym.scancode=convertKeyMap(e.key.keysym.scancode);
      if(e.key.keysym.scancode==SDL_SCANCODE_UNKNOWN || e.key.keysym.scancode ==999){
        return;
      }
#endif
#endif //GCW0

			if (e.key.keysym.scancode == SDL_SCANCODE_F11) {
#ifdef EMSCRIPTEN //in web browsers, F11 is commonly occupied. Use CTRL+F11
			if ((e.key.keysym.mod == KMOD_LCTRL) || (e.key.keysym.mod == KMOD_RCTRL)) {
#endif 
=======
#if SDL_MAJOR_VERSION == 1
			if (e.key.keysym.sym == SDLK_F11) {
#else
			if (e.key.keysym.scancode == SDL_SCANCODE_F11) {
#endif
>>>>>>> upstream/master
				if (menuvram == NULL) {
					sysmenu_menuopen(0, 0, 0);
				}
				else {
					menubase_close();
				}
			}
#ifdef EMSCRIPTEN
			}
			else if (e.key.keysym.scancode == SDL_SCANCODE_F12) {
				if ((e.key.keysym.mod == KMOD_LCTRL) || (e.key.keysym.mod == KMOD_RCTRL)) {
					//use CTRL+F12 to lock mouse like win32 builds do
					mousemng_toggle(MOUSEPROC_SYSTEM);
				}
			}
#endif
			else {
#if SDL_MAJOR_VERSION == 1
				sdlkbd_keydown(e.key.keysym.sym);
#else
				sdlkbd_keydown(e.key.keysym.scancode);
#endif
			}
			break;

		case SDL_KEYUP:
#if defined(GCW0)
#if SDL_MAJOR_VERSION != 1
      e.key.keysym.scancode=convertKeyMap(e.key.keysym.scancode);
      if(e.key.keysym.scancode==SDL_SCANCODE_UNKNOWN || e.key.keysym.scancode ==999){
        return;
      }
#endif
#endif //GCW0

#if SDL_MAJOR_VERSION == 1
      sdlkbd_keyup(e.key.keysym.sym);
#else
      sdlkbd_keyup(e.key.keysym.scancode);
#endif
			break;

		case SDL_QUIT:
			task_avail = FALSE;
			break;
	}
#endif	/* __LIBRETRO__ */
}

BOOL taskmng_sleep(UINT32 tick) {

	UINT32	base;

	base = GETTICK();
	while((task_avail) && ((GETTICK() - base) < tick)) {
		taskmng_rol();
#if defined(__LIBRETRO__)
      retro_sleep(1);
#else	/* __LIBRETRO__ */
#ifndef EMSCRIPTEN
		SDL_Delay(1);
#else
		emscripten_sleep_with_yield(1);
#endif
#endif	/* __LIBRETRO__ */
	}
	return(task_avail);
}
