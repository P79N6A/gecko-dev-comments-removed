





#ifndef mozilla_dom_icc_IccChild_h
#define mozilla_dom_icc_IccChild_h

#include "mozilla/dom/icc/PIccChild.h"
#include "mozilla/dom/icc/PIccRequestChild.h"
#include "nsIIccService.h"

namespace mozilla {
namespace dom {

class IccInfo;

namespace icc {

class IccChild final : public PIccChild
                     , public nsIIcc
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIICC

  explicit IccChild();

void
  Init();

  void
  Shutdown();

protected:
  virtual void
  ActorDestroy(ActorDestroyReason why) override;

  virtual PIccRequestChild*
  AllocPIccRequestChild(const IccRequest& aRequest) override;

  virtual bool
  DeallocPIccRequestChild(PIccRequestChild* aActor) override;

  virtual bool
  RecvNotifyCardStateChanged(const uint32_t& aCardState) override;

  virtual bool
  RecvNotifyIccInfoChanged(const OptionalIccInfoData& aInfoData) override;

  virtual bool
  RecvNotifyStkCommand(const nsString& aStkProactiveCmd) override;

  virtual bool
  RecvNotifyStkSessionEnd() override;

private:
  ~IccChild();

  void
  UpdateIccInfo(const OptionalIccInfoData& aInfoData);

  bool
  SendRequest(const IccRequest& aRequest, nsIIccCallback* aRequestReply);

  nsCOMArray<nsIIccListener> mListeners;
  nsRefPtr<IccInfo> mIccInfo;
  uint32_t mCardState;
  bool mIsAlive;
};

class IccRequestChild final : public PIccRequestChild
{
public:
  explicit IccRequestChild(nsIIccCallback* aRequestReply);

protected:
  virtual bool
  Recv__delete__(const IccReply& aReply) override;

private:
  virtual ~IccRequestChild() {
    MOZ_COUNT_DTOR(IccRequestChild);
  }

  nsCOMPtr<nsIIccCallback> mRequestReply;
};

} 
} 
} 

#endif 
