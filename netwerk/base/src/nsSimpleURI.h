




#ifndef nsSimpleURI_h__
#define nsSimpleURI_h__

#include "mozilla/MemoryReporting.h"
#include "nsIURI.h"
#include "nsISerializable.h"
#include "nsString.h"
#include "nsIClassInfo.h"
#include "nsIMutable.h"
#include "nsISizeOf.h"
#include "nsIIPCSerializableURI.h"

#define NS_THIS_SIMPLEURI_IMPLEMENTATION_CID         \
{ /* 0b9bb0c2-fee6-470b-b9b9-9fd9462b5e19 */         \
    0x0b9bb0c2,                                      \
    0xfee6,                                          \
    0x470b,                                          \
    {0xb9, 0xb9, 0x9f, 0xd9, 0x46, 0x2b, 0x5e, 0x19} \
}

class nsSimpleURI : public nsIURI,
                    public nsISerializable,
                    public nsIClassInfo,
                    public nsIMutable,
                    public nsISizeOf,
                    public nsIIPCSerializableURI
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIURI
    NS_DECL_NSISERIALIZABLE
    NS_DECL_NSICLASSINFO
    NS_DECL_NSIMUTABLE
    NS_DECL_NSIIPCSERIALIZABLEURI

    

    nsSimpleURI();
    virtual ~nsSimpleURI();

    
    
    
    
    
    
    
    virtual size_t SizeOfExcludingThis(mozilla::MallocSizeOf aMallocSizeOf) const;
    virtual size_t SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf) const;

protected:
    
    enum RefHandlingEnum {
        eIgnoreRef,
        eHonorRef
    };

    
    virtual nsresult EqualsInternal(nsIURI* other,
                                    RefHandlingEnum refHandlingMode,
                                    bool* result);

    
    
    
    bool EqualsInternal(nsSimpleURI* otherUri, RefHandlingEnum refHandlingMode);

    
    
    
    virtual nsSimpleURI* StartClone(RefHandlingEnum refHandlingMode);

    
    virtual nsresult CloneInternal(RefHandlingEnum refHandlingMode,
                                   nsIURI** clone);
    
    nsCString mScheme;
    nsCString mPath; 
    nsCString mRef;  
    bool mMutable;
    bool mIsRefValid; 
};

#endif
