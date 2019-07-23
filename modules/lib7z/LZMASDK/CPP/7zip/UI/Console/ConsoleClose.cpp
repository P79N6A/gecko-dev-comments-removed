

#include "StdAfx.h"

#include "ConsoleClose.h"

static int g_BreakCounter = 0;
static const int kBreakAbortThreshold = 2;

namespace NConsoleClose {

#ifndef UNDER_CE
static BOOL WINAPI HandlerRoutine(DWORD ctrlType)
{
  if (ctrlType == CTRL_LOGOFF_EVENT)
  {
    
    return TRUE;
  }

  g_BreakCounter++;
  if (g_BreakCounter < kBreakAbortThreshold)
    return TRUE;
  return FALSE;
  









}
#endif

bool TestBreakSignal()
{
  #ifdef UNDER_CE
  return false;
  #else
  



  return (g_BreakCounter > 0);
  #endif
}

void CheckCtrlBreak()
{
  if (TestBreakSignal())
    throw CCtrlBreakException();
}

CCtrlHandlerSetter::CCtrlHandlerSetter()
{
  #ifndef UNDER_CE
  if(!SetConsoleCtrlHandler(HandlerRoutine, TRUE))
    throw "SetConsoleCtrlHandler fails";
  #endif
}

CCtrlHandlerSetter::~CCtrlHandlerSetter()
{
  #ifndef UNDER_CE
  if(!SetConsoleCtrlHandler(HandlerRoutine, FALSE))
    throw "SetConsoleCtrlHandler fails";
  #endif
}

}
