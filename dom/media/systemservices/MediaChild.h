





#ifndef mozilla_MediaChild_h
#define mozilla_MediaChild_h

#include "mozilla/dom/ContentChild.h"
#include "mozilla/media/PMediaChild.h"
#include "mozilla/media/PMediaParent.h"
#include "nsIIPCBackgroundChildCreateCallback.h"
#include "MediaUtils.h"

namespace mozilla {
namespace media {










template<typename ValueType>
class ChildPledge : public Pledge<ValueType>,
                    public nsIIPCBackgroundChildCreateCallback
{
  NS_DECL_NSIIPCBACKGROUNDCHILDCREATECALLBACK
  NS_DECL_ISUPPORTS
public:
  explicit ChildPledge();
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
public:
  Child();
  virtual ~Child();
};

PMediaChild* CreateMediaChild();

} 
} 

#endif  
