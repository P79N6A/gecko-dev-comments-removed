





#ifndef mozilla_MediaChild_h
#define mozilla_MediaChild_h

#include "mozilla/dom/ContentChild.h"
#include "mozilla/media/PMediaChild.h"
#include "mozilla/media/PMediaParent.h"
#include "nsIIPCBackgroundChildCreateCallback.h"
#include "MediaUtils.h"

namespace mozilla {
namespace media {










class Child;

template<typename ValueType>
class ChildPledge : public Pledge<ValueType>,
                    public nsIIPCBackgroundChildCreateCallback
{
  friend Child;
  NS_DECL_NSIIPCBACKGROUNDCHILDCREATECALLBACK
  NS_DECL_ISUPPORTS
public:
  explicit ChildPledge() {};
protected:
  virtual ~ChildPledge() {}
  virtual void Run(PMediaChild* aMedia) = 0;
};

already_AddRefed<ChildPledge<nsCString>>
GetOriginKey(const nsCString& aOrigin, bool aPrivateBrowsing);

already_AddRefed<ChildPledge<bool>>
SanitizeOriginKeys(const uint64_t& aSinceWhen);

class Child : public PMediaChild
{
  NS_INLINE_DECL_REFCOUNTING(Child)
public:
  Child();

  bool RecvGetOriginKeyResponse(const uint32_t& aRequestId, const nsCString& aKey);

  uint32_t AddRequestPledge(ChildPledge<nsCString>& aPledge);
  already_AddRefed<ChildPledge<nsCString>> RemoveRequestPledge(uint32_t aRequestId);
private:
  virtual ~Child();

  typedef std::pair<uint32_t,nsRefPtr<ChildPledge<nsCString>>> PledgeEntry;
  static uint32_t sRequestCounter;
  nsTArray<PledgeEntry> mRequestPledges;
};

PMediaChild* AllocPMediaChild();
bool DeallocPMediaChild(PMediaChild *aActor);

} 
} 

#endif  
