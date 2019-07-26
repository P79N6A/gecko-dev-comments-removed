



#ifndef mozilla_ipc_nsiipcbackgroundchildcreatecallback_h
#define mozilla_ipc_nsiipcbackgroundchildcreatecallback_h

#include "mozilla/Attributes.h"
#include "nsISupports.h"

namespace mozilla {
namespace ipc {

class PBackgroundChild;

} 
} 

#define NS_IIPCBACKGROUNDCHILDCREATECALLBACK_IID                               \
  {0x4de01707, 0x70e3, 0x4181, {0xbc, 0x9f, 0xa3, 0xec, 0xfe, 0x74, 0x1a, 0xe3}}

class NS_NO_VTABLE nsIIPCBackgroundChildCreateCallback : public nsISupports
{
public:
  typedef mozilla::ipc::PBackgroundChild PBackgroundChild;

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IIPCBACKGROUNDCHILDCREATECALLBACK_IID)

  
  
  
  
  
  virtual void
  ActorCreated(PBackgroundChild*) = 0;

  
  
  
  
  virtual void
  ActorFailed() = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIIPCBackgroundChildCreateCallback,
                              NS_IIPCBACKGROUNDCHILDCREATECALLBACK_IID)

#define NS_DECL_NSIIPCBACKGROUNDCHILDCREATECALLBACK                            \
  virtual void                                                                 \
  ActorCreated(mozilla::ipc::PBackgroundChild*) MOZ_OVERRIDE;                  \
  virtual void                                                                 \
  ActorFailed() MOZ_OVERRIDE;

#define NS_FORWARD_NSIIPCBACKGROUNDCHILDCREATECALLBACK(_to)                    \
  virtual void                                                                 \
  ActorCreated(mozilla::ipc::PBackgroundChild* aActor) MOZ_OVERRIDE            \
  { _to ActorCreated(aActor); }                                                \
  virtual void                                                                 \
  ActorFailed() MOZ_OVERRIDE                                                   \
  { _to ActorFailed(); }

#define NS_FORWARD_SAFE_NSIIPCBACKGROUNDCHILDCREATECALLBACK(_to)               \
  virtual void                                                                 \
  ActorCreated(mozilla::ipc::PBackgroundChild* aActor) MOZ_OVERRIDE            \
  { if (_to) { _to->ActorCreated(aActor); } }                                  \
  virtual void                                                                 \
  ActorFailed() MOZ_OVERRIDE                                                   \
  { if (_to) { _to->ActorFailed(); } }

#endif 
