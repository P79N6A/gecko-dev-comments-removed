





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

struct MSGResult;

class WindowHook {
public:

  
  typedef bool (*Callback)(void *aContext, HWND hWnd, UINT nMsg,
                             WPARAM wParam, LPARAM lParam, LRESULT *aResult);

  nsresult AddHook(UINT nMsg, Callback callback, void *context);
  nsresult RemoveHook(UINT nMsg, Callback callback, void *context);
  nsresult AddMonitor(UINT nMsg, Callback callback, void *context);
  nsresult RemoveMonitor(UINT nMsg, Callback callback, void *context);

private:
  struct CallbackData {
    Callback cb;
    void *context;

    CallbackData() : cb(nullptr), context(nullptr) {}
    CallbackData(Callback cb, void *ctx) : cb(cb), context(ctx) {}
    bool Invoke(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam,
                  LRESULT *aResult);
    bool operator== (const CallbackData &rhs) const {
      return cb == rhs.cb && context == rhs.context;
    }
    bool operator!= (const CallbackData &rhs) const {
      return !(*this == rhs);
    }
    operator bool () const {
      return !!cb;
    }
  };

  typedef nsTArray<CallbackData> CallbackDataArray;
  struct MessageData {
    UINT nMsg;
    CallbackData hook;
    CallbackDataArray monitors;
  };

  bool Notify(HWND hWnd, UINT nMsg, WPARAM wParam, LPARAM lParam,
              MSGResult& aResult);

  MessageData *Lookup(UINT nMsg);
  MessageData *LookupOrCreate(UINT nMsg);
  void DeleteIfEmpty(MessageData *data);

  typedef nsTArray<MessageData> MessageDataArray;
  MessageDataArray mMessageData;

  
  friend class ::nsWindow;
};

}
}

#endif 
