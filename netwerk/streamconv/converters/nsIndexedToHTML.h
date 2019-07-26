




#ifndef ____nsindexedtohtml___h___
#define ____nsindexedtohtml___h___

#include "nsCOMPtr.h"
#include "nsString.h"
#include "nsIStreamConverter.h"
#include "nsIDirIndexListener.h"

#define NS_NSINDEXEDTOHTMLCONVERTER_CID \
{ 0xcf0f71fd, 0xfafd, 0x4e2b, {0x9f, 0xdc, 0x13, 0x4d, 0x97, 0x2e, 0x16, 0xe2} }

class nsIDateTimeFormat;
class nsIStringBundle;
class nsITextToSubURI;

class nsIndexedToHTML : public nsIStreamConverter,
                        public nsIDirIndexListener
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSISTREAMCONVERTER
    NS_DECL_NSIREQUESTOBSERVER
    NS_DECL_NSISTREAMLISTENER
    NS_DECL_NSIDIRINDEXLISTENER

    nsIndexedToHTML();

    nsresult Init(nsIStreamListener *aListener);

    static nsresult
    Create(nsISupports *aOuter, REFNSIID aIID, void **aResult);

protected:
    
    void FormatSizeString(int64_t inSize, nsCString& outSizeString);
    nsresult SendToListener(nsIRequest* aRequest, nsISupports *aContext, const nsACString &aBuffer);
    
    nsresult DoOnStartRequest(nsIRequest* request, nsISupports *aContext,
                              nsCString& aBuffer);

protected:
    nsCOMPtr<nsIDirIndexParser>     mParser;
    nsCOMPtr<nsIStreamListener>     mListener; 

    nsCOMPtr<nsIDateTimeFormat> mDateTime;
    nsCOMPtr<nsIStringBundle> mBundle;

    nsCOMPtr<nsITextToSubURI> mTextToSubURI;

private:
    
    bool mExpectAbsLoc;

    virtual ~nsIndexedToHTML();
};

#endif

