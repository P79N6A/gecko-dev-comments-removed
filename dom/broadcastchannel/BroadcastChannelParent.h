





#ifndef mozilla_dom_BroadcastChannelParent_h
#define mozilla_dom_BroadcastChannelParent_h

#include "mozilla/dom/PBroadcastChannelParent.h"

namespace mozilla {

namespace ipc {
class BackgroundParentImpl;
class PrincipalInfo;
}

namespace dom {

class BroadcastChannelService;

class BroadcastChannelParent final : public PBroadcastChannelParent
{
  friend class mozilla::ipc::BackgroundParentImpl;

  typedef mozilla::ipc::PrincipalInfo PrincipalInfo;

public:
  void CheckAndDeliver(const ClonedMessageData& aData,
                       const nsCString& aOrigin,
                       const uint64_t aAppId,
                       const bool aIsInBrowserElement,
                       const nsString& aChannel,
                       bool aPrivateBrowsing);

private:
  BroadcastChannelParent(const PrincipalInfo& aPrincipalInfo,
                         const nsACString& aOrigin,
                         const nsAString& aChannel,
                         bool aPrivateBrowsing);
  ~BroadcastChannelParent();

  virtual bool
  RecvPostMessage(const ClonedMessageData& aData) override;

  virtual bool RecvClose() override;

  virtual void ActorDestroy(ActorDestroyReason aWhy) override;

  nsRefPtr<BroadcastChannelService> mService;
  nsCString mOrigin;
  nsString mChannel;
  uint64_t mAppId;
  bool mIsInBrowserElement;
  bool mPrivateBrowsing;
};

} 
} 

#endif 
