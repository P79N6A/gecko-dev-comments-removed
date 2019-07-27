



#ifndef mozilla_ipc_backgroundchild_h__
#define mozilla_ipc_backgroundchild_h__

#include "base/process.h"
#include "mozilla/Attributes.h"
#include "mozilla/ipc/Transport.h"

class nsIDOMBlob;
class nsIIPCBackgroundChildCreateCallback;

namespace mozilla {
namespace dom {

class BlobImpl;
class ContentChild;
class ContentParent;
class PBlobChild;

} 

namespace ipc {

class PBackgroundChild;






















class BackgroundChild final
{
  friend class mozilla::dom::ContentChild;
  friend class mozilla::dom::ContentParent;

  typedef base::ProcessId ProcessId;
  typedef mozilla::ipc::Transport Transport;

public:
  
  static PBackgroundChild*
  GetForCurrentThread();

  
  static bool
  GetOrCreateForCurrentThread(nsIIPCBackgroundChildCreateCallback* aCallback);

  static mozilla::dom::PBlobChild*
  GetOrCreateActorForBlob(PBackgroundChild* aBackgroundActor,
                          nsIDOMBlob* aBlob);

  static mozilla::dom::PBlobChild*
  GetOrCreateActorForBlobImpl(PBackgroundChild* aBackgroundActor,
                              mozilla::dom::BlobImpl* aBlobImpl);

  
  static void
  CloseForCurrentThread();

private:
  
  static void
  Startup();

  
  static PBackgroundChild*
  Alloc(Transport* aTransport, ProcessId aOtherProcess);
};

} 
} 

#endif 
