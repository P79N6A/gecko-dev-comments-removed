






#ifndef mozilla_dom_cellbroadcast_CellBroadcastParent_h
#define mozilla_dom_cellbroadcast_CellBroadcastParent_h

#include "mozilla/dom/cellbroadcast/PCellBroadcastParent.h"
#include "nsICellBroadcastService.h"

namespace mozilla {
namespace dom {
namespace cellbroadcast {

class CellBroadcastParent MOZ_FINAL : public PCellBroadcastParent
                                    , public nsICellBroadcastListener
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSICELLBROADCASTLISTENER

  bool Init();

private:
  
  ~CellBroadcastParent() {};

  virtual void ActorDestroy(ActorDestroyReason aWhy) MOZ_OVERRIDE;
};

} 
} 
} 

#endif 
