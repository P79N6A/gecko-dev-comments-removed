






































#ifndef __mozilla_WindowHook_h__
#define __mozilla_WindowHook_h__

#include <windows.h>

#include <nsHashKeys.h>
#include <nsClassHashtable.h>
#include <nsTArray.h>

#include "nsAppShell.h"

class nsWindow;

namespace mozilla {
namespace widget {

class WindowHook {
public:

  
  typedef PRBool (*Callback)(void *aContext, HWND hWnd, UINT nMsg,
                             WPARAM wParam, LPARAM lParam, LRESULT *aResult);

  nsresult AddHook(UINT nMsg, Callback callback, void *context);
  nsresult RemoveHook(UINT nMsg, Callback callback, void *context);
  nsresult AddMonitor(UINT nMsg, Callback callback, void *context);
  nsresult RemoveMonitor(UINT nMsg, Callback callback, void *context);

private:
  struct CallbackData {
    Callback cb;
    void *context;

    CallbackData() : cb(nsnull), context(nsnull) {}
    CallbackData(Callback cb, void *ctx) : cb(cb), context(ctx) {}
    PRBool Invoke(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam,
                  LRESULT *aResult);
    PRBool operator== (const CallbackData &rhs) const {
      return cb == rhs.cb && context == rhs.context;
    }
    PRBool operator!= (const CallbackData &rhs) const {
      return !(*this == rhs);
    }
    operator PRBool () const {
      return !!cb;
    }
  };

  typedef nsTArray<CallbackData> CallbackDataArray;
  struct MessageData {
    UINT nMsg;
    CallbackData hook;
    CallbackDataArray monitors;
  };

  PRBool Notify(HWND hWnd, UINT nMsg, WPARAM wParam, LPARAM lParam,
                LRESULT *aResult);

  MessageData *Lookup(UINT nMsg);
  MessageData *LookupOrCreate(UINT nMsg);
  void DeleteIfEmpty(MessageData *data);

  typedef nsTArray<MessageData> MessageDataArray;
  MessageDataArray mMessageData;

  
  friend nsWindow;
};

}
}

#endif 
