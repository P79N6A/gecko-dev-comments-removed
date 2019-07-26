






#ifndef nsStructuredCloneContainer_h__
#define nsStructuredCloneContainer_h__

#include "nsIStructuredCloneContainer.h"
#include "jsapi.h"
#include "mozilla/Attributes.h"

#define NS_STRUCTUREDCLONECONTAINER_CONTRACTID \
  "@mozilla.org/docshell/structured-clone-container;1"
#define NS_STRUCTUREDCLONECONTAINER_CID \
{ /* 38bd0634-0fd4-46f0-b85f-13ced889eeec */       \
  0x38bd0634,                                      \
  0x0fd4,                                          \
  0x46f0,                                          \
  {0xb8, 0x5f, 0x13, 0xce, 0xd8, 0x89, 0xee, 0xec} \
}

class nsStructuredCloneContainer MOZ_FINAL : public nsIStructuredCloneContainer
{
  public:
    nsStructuredCloneContainer();
    ~nsStructuredCloneContainer();

    NS_DECL_ISUPPORTS
    NS_DECL_NSISTRUCTUREDCLONECONTAINER

  private:
    uint64_t* mData;

    
    size_t mSize;
    uint32_t mVersion;
};

#endif
