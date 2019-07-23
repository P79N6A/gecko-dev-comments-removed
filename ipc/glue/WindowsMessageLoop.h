






































#ifndef IPC_GLUE_WINDOWSMESSAGELOOP_H
#define IPC_GLUE_WINDOWSMESSAGELOOP_H


#include <windows.h>

#include "base/basictypes.h"
#include "nsTraceRefCnt.h"

namespace mozilla {
namespace ipc {
namespace windows {

class DeferredMessage
{
public:
  DeferredMessage()
  {
    MOZ_COUNT_CTOR(DeferredMessage);
  }

  virtual ~DeferredMessage()
  {
    MOZ_COUNT_DTOR(DeferredMessage);
  }

  virtual void Run() = 0;
};




class DeferredSendMessage : public DeferredMessage
{
public:
  DeferredSendMessage(HWND aHWnd,
                      UINT aMessage,
                      WPARAM aWParam,
                      LPARAM aLParam)
    : hWnd(aHWnd),
      message(aMessage),
      wParam(aWParam),
      lParam(aLParam)
  { }

  virtual void Run();

protected:
    HWND hWnd;
    UINT message;
    WPARAM wParam;
    LPARAM lParam;
};



class DeferredRedrawMessage : public DeferredMessage
{
public:
  DeferredRedrawMessage(HWND aHWnd,
                        UINT aFlags)
    : hWnd(aHWnd),
      flags(aFlags)
  { }

  virtual void Run();

private:
  HWND hWnd;
  UINT flags;
};


class DeferredUpdateMessage : public DeferredMessage
{
public:
  DeferredUpdateMessage(HWND aHWnd)
    : hWnd(aHWnd)
  { }

  virtual void Run();

private:
  HWND hWnd;
};



class DeferredSettingChangeMessage : public DeferredSendMessage
{
public:
  DeferredSettingChangeMessage(HWND aHWnd,
                               UINT aMessage,
                               WPARAM aWParam,
                               LPARAM aLParam);

  ~DeferredSettingChangeMessage();
private:
  wchar_t* lParamString;
};



class DeferredWindowPosMessage : public DeferredMessage
{
public:
  DeferredWindowPosMessage(HWND aHWnd,
                           UINT aFlags)
   : hWnd(aHWnd),
     flags(aFlags)
  { }

  virtual void Run();

private:
  HWND hWnd;
  UINT flags;
};

class DeferredNCActivateMessage : public DeferredSendMessage
{
public:
  DeferredNCActivateMessage(HWND aHWnd,
                            UINT aMessage,
                            WPARAM aWParam,
                            LPARAM aLParam);

  ~DeferredNCActivateMessage();

private:
  HRGN region;
};

} 
} 
} 

#endif 
