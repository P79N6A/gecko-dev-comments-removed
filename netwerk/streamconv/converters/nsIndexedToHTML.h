




































#ifndef ____nsindexedtohtml___h___
#define ____nsindexedtohtml___h___

#include "nsCOMPtr.h"
#include "nsVoidArray.h"
#include "nsIFactory.h"
#include "nsString.h"
#include "nsIStreamConverter.h"
#include "nsXPIDLString.h"
#include "nsIDirIndexListener.h"
#include "nsIDateTimeFormat.h"
#include "nsIStringBundle.h"
#include "nsIStringStream.h"
#include "nsITextToSubURI.h"
#include "nsICharsetConverterManager.h"

#define NS_NSINDEXEDTOHTMLCONVERTER_CID \
{ 0xcf0f71fd, 0xfafd, 0x4e2b, {0x9f, 0xdc, 0x13, 0x4d, 0x97, 0x2e, 0x16, 0xe2} }


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
    virtual ~nsIndexedToHTML();

    nsresult Init(nsIStreamListener *aListener);

    static NS_METHOD
    Create(nsISupports *aOuter, REFNSIID aIID, void **aResult);

protected:
    
    void FormatSizeString(PRInt64 inSize, nsString& outSizeString);
    nsresult FormatInputStream(nsIRequest* aRequest, nsISupports *aContext, const nsAString &aBuffer); 

protected:
    nsCOMPtr<nsIDirIndexParser>     mParser;
    nsCOMPtr<nsIStreamListener>     mListener; 

    nsCOMPtr<nsIDateTimeFormat> mDateTime;
    nsCOMPtr<nsIStringBundle> mBundle;

    nsCOMPtr<nsITextToSubURI> mTextToSubURI;
    nsCOMPtr<nsIUnicodeEncoder> mUnicodeEncoder;

private:
    
    PRBool mExpectAbsLoc;
};

#endif

