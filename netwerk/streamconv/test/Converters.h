#ifndef Converters_h___
#define Converters_h___

#include "nsIStreamConverter.h"
#include "nsIFactory.h"
#include "nsCOMPtr.h"
#include "nsStringAPI.h"

#include <algorithm>









extern const nsCID kTestConverterCID;

class TestConverter : public nsIStreamConverter {
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIREQUESTOBSERVER
    NS_DECL_NSISTREAMLISTENER

    TestConverter();
    virtual ~TestConverter() {}

    
    NS_IMETHOD Convert(nsIInputStream *aFromStream, const char *aFromType, 
                       const char *aToType, nsISupports *ctxt, nsIInputStream **_retval);


    NS_IMETHOD AsyncConvertData(const char *aFromType, const char *aToType, 
                                nsIStreamListener *aListener, nsISupports *ctxt);

    
    nsCOMPtr<nsIStreamListener> mListener;
    nsCString fromType;
    nsCString toType;
};

nsresult CreateTestConverter(nsISupports* aOuter, REFNSIID aIID, void** aResult);

static inline uint32_t
saturated(uint64_t aValue)
{
    return (uint32_t) std::min(aValue, (uint64_t) UINT32_MAX);
}

#endif 
