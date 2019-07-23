




































#include <windows.h>
#include <windowsx.h>
#include <process.h>

#include "thread.h"
#include "dbg.h"

static DWORD WINAPI ThreadFunction(void * lp)
{
  if (lp == NULL)
    return 0L;

  CThread * thread = (CThread *)lp;
  BOOL res = thread->init();
  thread->setInitEvent();

  if(res)
    thread->run();

  thread->shut();
  DWORD ret;
  GetExitCodeThread(thread->getHandle(), &ret);
  thread->setShutEvent();
  return ret;
}

CThread::CThread(DWORD aInitTimeOut, DWORD aShutTimeOut) :
  mThread(NULL),
  mID(0),
  mState(ts_dead),
  mEventThreadInitializationFinished(NULL),
  mEventThreadShutdownFinished(NULL),
  mInitTimeOut(aInitTimeOut),
  mShutTimeOut(aShutTimeOut),
  mEventDo(NULL),
  mAction(0)
{
  dbgOut1("CThread::CThread");
}

CThread::~CThread()
{
  dbgOut1("CThread::~CThread");
}

BOOL CThread::open(CThread * aThread)
{
  dbgOut1("CThread::open");

  mEventThreadInitializationFinished = CreateEvent(NULL, TRUE, FALSE, NULL);
  mEventThreadShutdownFinished = CreateEvent(NULL, TRUE, FALSE, NULL);
  mEventDo = CreateEvent(NULL, TRUE, FALSE, NULL);

  mThread = (HANDLE)::_beginthreadex(0, 1024, (UINT (__stdcall *)(void *))ThreadFunction, aThread, 0L, (UINT *)&mID);

  if(mThread == NULL)
    return FALSE;

  SetThreadPriority(mThread, THREAD_PRIORITY_NORMAL);

  if(mEventThreadInitializationFinished != NULL)
    WaitForSingleObject(mEventThreadInitializationFinished, mInitTimeOut);

  return TRUE;
}

void CThread::close(CThread * aThread)
{
  switch (mState) {
    case ts_dead:
      break;
    case ts_ready:
      mState = ts_dead;
      SetEvent(mEventDo);
      if(mEventThreadShutdownFinished != NULL)
        WaitForSingleObject(mEventThreadShutdownFinished, mShutTimeOut);
      break;
    case ts_busy:
    {
      aThread->shut();
      DWORD ret;
      GetExitCodeThread(aThread->getHandle(), &ret);
      TerminateThread(mThread, ret);
      mState = ts_dead;
      break;
    }
    default:
      break;
  }

  mThread = NULL;
  CloseHandle(mEventThreadInitializationFinished);
  CloseHandle(mEventThreadShutdownFinished);
  CloseHandle(mEventDo);
  dbgOut1("CThread::close");
}

BOOL CThread::setInitEvent()
{
  return SetEvent(mEventThreadInitializationFinished);
}

BOOL CThread::setShutEvent()
{
  return SetEvent(mEventThreadShutdownFinished);
}

BOOL CThread::isAlive()
{
  return (mThread != NULL);
}

BOOL CThread::isBusy()
{
  return (mState == ts_busy);
}

HANDLE CThread::getHandle()
{
  return mThread;
}

void CThread::run()
{
  mState = ts_ready;

  while(mState != ts_dead) {
    WaitForSingleObject(mEventDo, INFINITE);
    
    ResetEvent(mEventDo);
    if(mState == ts_dead)
      return;
    mState = ts_busy;
    dispatch();
    mState = ts_ready;
  }
  mState = ts_dead;
}

BOOL CThread::tryAction(int aAction)
{
  if(mState != ts_ready)
    return FALSE;

  mAction = aAction;
  SetEvent(mEventDo);

  return TRUE;
}

BOOL CThread::doAction(int aAction)
{
  while(mState != ts_ready) {
    
    if(mState == ts_dead)
      return FALSE;
    Sleep(0);
  }

  mState = ts_busy;
  mAction = aAction;
  
  SetEvent(mEventDo);

  return TRUE;
}
