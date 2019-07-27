



#ifndef mozilla_ipc_backgroundparent_h__
#define mozilla_ipc_backgroundparent_h__

#include "base/process.h"
#include "mozilla/Attributes.h"
#include "mozilla/ipc/Transport.h"

template <class> struct already_AddRefed;

namespace mozilla {
namespace dom {

class ContentParent;
class FileImpl;
class PBlobParent;

} 

namespace ipc {

class PBackgroundParent;



class BackgroundParent final
{
  friend class mozilla::dom::ContentParent;

  typedef base::ProcessId ProcessId;
  typedef mozilla::dom::ContentParent ContentParent;
  typedef mozilla::dom::FileImpl FileImpl;
  typedef mozilla::ipc::Transport Transport;

public:
  
  
  
  
  static bool
  IsOtherProcessActor(PBackgroundParent* aBackgroundActor);

  
  
  
  
  
  
  
  
  static already_AddRefed<ContentParent>
  GetContentParent(PBackgroundParent* aBackgroundActor);

  static mozilla::dom::PBlobParent*
  GetOrCreateActorForBlobImpl(PBackgroundParent* aBackgroundActor,
                              FileImpl* aBlobImpl);

  
  
  
  
  static intptr_t
  GetRawContentParentForComparison(PBackgroundParent* aBackgroundActor);

private:
  
  static PBackgroundParent*
  Alloc(ContentParent* aContent,
        Transport* aTransport,
        ProcessId aOtherProcess);
};


bool
IsOnBackgroundThread();

#ifdef DEBUG


void
AssertIsOnBackgroundThread();

#else

inline void
AssertIsOnBackgroundThread()
{ }

#endif 

} 
} 

#endif 
