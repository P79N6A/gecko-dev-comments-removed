





































#ifndef WindowDbg_h__
#define WindowDbg_h__





#include "nsWindowDefs.h"


















#if defined(EVENT_DEBUG_OUTPUT)
#define SHOW_REPEAT_EVENTS      PR_TRUE
#define SHOW_MOUSEMOVE_EVENTS   PR_FALSE
#endif 

#if defined(POPUP_ROLLUP_DEBUG_OUTPUT) || defined(EVENT_DEBUG_OUTPUT) || 1
void PrintEvent(UINT msg, bool aShowAllEvents, bool aShowMouseMoves);
#endif 

#if defined(POPUP_ROLLUP_DEBUG_OUTPUT)
typedef struct {
  char * mStr;
  int    mId;
} MSGFEventMsgInfo;

#define DISPLAY_NMM_PRT(_arg) PR_LOG(gWindowsLog, PR_LOG_ALWAYS, ((_arg)));
#else
#define DISPLAY_NMM_PRT(_arg)
#endif 

#if defined(DEBUG)
void DDError(const char *msg, HRESULT hr);
#endif 

#if defined(DEBUG_VK)
bool is_vk_down(int vk);
#define IS_VK_DOWN is_vk_down
#else
#define IS_VK_DOWN(a) (GetKeyState(a) < 0)
#endif 

#endif 
