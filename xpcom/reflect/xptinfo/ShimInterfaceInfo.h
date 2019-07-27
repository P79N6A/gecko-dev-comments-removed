






#ifndef ShimInterfaceInfo_h
#define ShimInterfaceInfo_h

#include "mozilla/Attributes.h"
#include "nsIInterfaceInfo.h"
#include "nsString.h"
#include "nsID.h"
#include "nsTArray.h"
#include "xptinfo.h"
#include "nsAutoPtr.h"
#include "js/RootingAPI.h"

namespace mozilla {
namespace dom {
struct NativePropertyHooks;
}
}

class ShimInterfaceInfo final : public nsIInterfaceInfo
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIINTERFACEINFO

    
    
    static already_AddRefed<ShimInterfaceInfo>
    MaybeConstruct(const char* aName, JSContext* cx);

private:
    ShimInterfaceInfo(const nsIID& aIID,
                      const char* aName,
                      const mozilla::dom::NativePropertyHooks* aNativePropHooks);

    ~ShimInterfaceInfo() {}

private:
    nsIID mIID;
    nsAutoCString mName;
    const mozilla::dom::NativePropertyHooks* mNativePropHooks;
};

#endif
