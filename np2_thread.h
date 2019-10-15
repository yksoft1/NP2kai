/* === NP2 threading library === (c) 2019 AZO */

#ifndef _NP2_THREAD_H_
#define _NP2_THREAD_H_

#ifdef SUPPORT_NP2_THREAD

#include <stdlib.h>

#if defined(NP2_THREAD_WIN)
#include <windows.h>
#include <process.h>
#elif defined(NP2_THREAD_POSIX)
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#elif defined(NP2_THREAD_SDL2)
#if defined(USE_SDL_CONFIG)
#include "SDL.h"
#else
#include <SDL.h>
#endif
#elif defined(NP2_THREAD_LR)
#include <rthreads/rthreads.h>
#include <retro_timers.h>
#include "sdl2/libretro/rsemaphore.h"
#endif

#if defined(NP2_THREAD_WIN)
typedef HANDLE NP2_Thread_t;
#elif defined(NP2_THREAD_POSIX)
typedef pthread_t NP2_Thread_t;
#elif defined(NP2_THREAD_SDL2)
typedef SDL_Thread* NP2_Thread_t;
#elif defined(NP2_THREAD_LR)
typedef sthread_t* NP2_Thread_t;
#endif

#ifndef TRACEOUT
#define TRACEOUT(s)
#endif

/* --- thread --- */

/* for caller */
void NP2_Thread_Create(NP2_Thread_t* pth, void*(*thread)(void*), void* param);
/* for caller */
void NP2_Thread_Destroy(NP2_Thread_t* pth);
/* for callee */
int NP2_Thread_Exit(void* retval);
/* for caller */
void NP2_Thread_Wait(NP2_Thread_t* pth, void** retval);
/* for caller */
void NP2_Thread_Detach(NP2_Thread_t* pth);

/* --- semaphore --- */

#if defined(NP2_THREAD_WIN)
typedef HANDLE NP2_Semaphore_t;
#elif defined(NP2_THREAD_POSIX)
typedef sem_t NP2_Semaphore_t;
#elif defined(NP2_THREAD_SDL2)
typedef SDL_sem* NP2_Semaphore_t;
#elif defined(NP2_THREAD_LR)
typedef ssem_t* NP2_Semaphore_t;
#endif

/* for caller */
void NP2_Semaphore_Create(NP2_Semaphore_t* psem, const unsigned int initcount);
/* for caller */
void NP2_Semaphore_Destroy(NP2_Semaphore_t* psem);
/* for caller/callee */
void NP2_Semaphore_Wait(NP2_Semaphore_t* psem);
/* for caller/callee */
void NP2_Semaphore_Release(NP2_Semaphore_t* psem);

/* --- wait queue --- */

typedef enum {
	NP2_WAITQUEUE_TYPE_RING = 0,
	NP2_WAITQUEUE_TYPE_RINGINT,
	NP2_WAITQUEUE_TYPE_LIST,
} NP2_WaitQueue_Type_t;

typedef struct NP2_WaitQueue_List_Item_t_ {
  struct NP2_WaitQueue_List_Item_t_* next;
  void* param;
} NP2_WaitQueue_List_Item_t;

typedef struct NP2_WaitQueue_Ring_t_ {
  NP2_WaitQueue_Type_t type;
  void* params;
  unsigned int maxcount;
  unsigned int current;
  unsigned int queued;
} NP2_WaitQueue_Ring_t;

typedef struct NP2_WaitQueue_List_t_ {
  NP2_WaitQueue_Type_t type;
  NP2_WaitQueue_List_Item_t* first;
  NP2_WaitQueue_List_Item_t* last;
} NP2_WaitQueue_List_t;

typedef union NP2_WaitQueue_t_ {
  NP2_WaitQueue_Ring_t ring;
  NP2_WaitQueue_List_t list;
} NP2_WaitQueue_t;

/* for caller (ring) */
void NP2_WaitQueue_Ring_Create(NP2_WaitQueue_t* pque, unsigned int maxcount);
/* for caller (ringint) */
void NP2_WaitQueue_RingInt_Create(NP2_WaitQueue_t* pque, unsigned int maxcount);
/* for caller (list) */
void NP2_WaitQueue_List_Create(NP2_WaitQueue_t* pque);
/* for caller (ring,ringint,list) */
void NP2_WaitQueue_Destroy(NP2_WaitQueue_t* pque);
/* for caller (ringint) */
void NP2_WaitQueue_RingInt_Append(NP2_WaitQueue_t* pque, NP2_Semaphore_t* psem, const int param);
/* for caller (ring,list) */
void NP2_WaitQueue_Append(NP2_WaitQueue_t* pque, NP2_Semaphore_t* psem, void* param);
/* for callee (ringint) */
void NP2_WaitQueue_RingInt_Shift(NP2_WaitQueue_t* pque, NP2_Semaphore_t* psem, int* param);
/* for callee (ring,list) */
void NP2_WaitQueue_Shift(NP2_WaitQueue_t* pque, NP2_Semaphore_t* psem, void** param);
/* for callee (ringint) */
void NP2_WaitQueue_RingInt_Shift_Wait(NP2_WaitQueue_t* pque, NP2_Semaphore_t* psem, int* param);
/* for callee (ring,list) */
void NP2_WaitQueue_Shift_Wait(NP2_WaitQueue_t* pque, NP2_Semaphore_t* psem, void** param);

/* --- sleep ms --- */

#if defined(NP2_THREAD_WIN)
#define NP2_Sleep_ms(ms) Sleep(ms);
#elif defined(NP2_THREAD_POSIX)
#define NP2_Sleep_ms(ms) usleep(ms * 1000);
#elif defined(NP2_THREAD_SDL2)
#define NP2_Sleep_ms(ms) SDL_Delay(ms);
#elif defined(NP2_THREAD_LR)
#define NP2_Sleep_ms(ms) retro_sleep(ms);
#endif

#endif  /* SUPPORT_NP2_THREAD */
#endif  /* _NP2_THREAD_H_ */

