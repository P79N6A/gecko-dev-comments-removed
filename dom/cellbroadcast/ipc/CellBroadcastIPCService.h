





#ifndef mozilla_dom_cellbroadcast_CellBroadcastIPCService_h
#define mozilla_dom_cellbroadcast_CellBroadcastIPCService_h

#include "mozilla/dom/cellbroadcast/PCellBroadcastChild.h"
#include "nsICellBroadcastService.h"
#include "nsCOMPtr.h"
#include "nsTArray.h"

namespace mozilla {
namespace dom {
namespace cellbroadcast {

class CellBroadcastIPCService final : public PCellBroadcastChild
                                    , public nsICellBroadcastService

{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSICELLBROADCASTSERVICE

  CellBroadcastIPCService();

  
  virtual bool
  RecvNotifyReceivedMessage(const uint32_t& aServiceId,
                            const uint32_t& aGsmGeographicalScope,
                            const uint16_t& aMessageCode,
                            const uint16_t& aMessageId,
                            const nsString& aLanguage,
                            const nsString& aBody,
                            const uint32_t& aMessageClass,
                            const uint64_t& aTimestamp,
                            const uint32_t& aCdmaServiceCategory,
                            const bool& aHasEtwsInfo,
                            const uint32_t& aEtwsWarningType,
                            const bool& aEtwsEmergencyUserAlert,
                            const bool& aEtwsPopup) override;

  virtual void ActorDestroy(ActorDestroyReason aWhy) override;

private:
  
  ~CellBroadcastIPCService();

  bool mActorDestroyed;
  nsTArray<nsCOMPtr<nsICellBroadcastListener>> mListeners;
};

} 
} 
} 

#endif 
