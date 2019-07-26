



#ifndef mozilla_ipc_backgroundparent_h__
#define mozilla_ipc_backgroundparent_h__

#include "base/process.h"
#include "mozilla/Attributes.h"
#include "mozilla/ipc/Transport.h"

namespace mozilla {
namespace dom {

class ContentParent;

} 

namespace ipc {

class PBackgroundParent;



class BackgroundParent MOZ_FINAL
{
  friend class mozilla::dom::ContentParent;

  typedef base::ProcessId ProcessId;
  typedef mozilla::dom::ContentParent ContentParent;
  typedef mozilla::ipc::Transport Transport;

private:
  
  static PBackgroundParent*
  Alloc(ContentParent* aContent,
        Transport* aTransport,
        ProcessId aOtherProcess);
};

bool
IsOnBackgroundThread();

void
AssertIsOnBackgroundThread()
#ifdef DEBUG
;
#else
{ }
#endif

} 
} 

#endif 
