




#ifndef ____nstxttohtmlconv___h___
#define ____nstxttohtmlconv___h___

#include "nsITXTToHTMLConv.h"
#include "nsCOMPtr.h"
#include "nsTArray.h"
#include "nsAutoPtr.h"
#include "nsIFactory.h"
#include "nsString.h"

#define NS_NSTXTTOHTMLCONVERTER_CID                         \
{ /* 9ef9fa14-1dd1-11b2-9d65-d72d6d1f025e */ \
    0x9ef9fa14, \
    0x1dd1, \
    0x11b2, \
    {0x9d, 0x65, 0xd7, 0x2d, 0x6d, 0x1f, 0x02, 0x5e} \
}


typedef struct convToken {
    nsString token;     
    nsString modText;   
    bool     prepend;   
} convToken;
    




























class nsTXTToHTMLConv : public nsITXTToHTMLConv {
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSISTREAMCONVERTER
    NS_DECL_NSITXTTOHTMLCONV
    NS_DECL_NSIREQUESTOBSERVER
    NS_DECL_NSISTREAMLISTENER

    nsTXTToHTMLConv();
    virtual ~nsTXTToHTMLConv();
    nsresult Init();

    
    static NS_METHOD
    Create(nsISupports *aOuter, REFNSIID aIID, void **aResult) {
        nsresult rv;
        if (aOuter)
            return NS_ERROR_NO_AGGREGATION;

        nsTXTToHTMLConv* _s = new nsTXTToHTMLConv();
        if (_s == nullptr)
            return NS_ERROR_OUT_OF_MEMORY;
        NS_ADDREF(_s);
        rv = _s->Init();
        if (NS_FAILED(rv)) {
            delete _s;
            return rv;
        }
        rv = _s->QueryInterface(aIID, aResult);
        NS_RELEASE(_s);
        return rv;
    }


protected:
    
    int32_t FindToken(int32_t cursor, convToken* *_retval);

    
    
    int32_t CatHTML(int32_t front, int32_t back);

    nsCOMPtr<nsIStreamListener>     mListener; 
    nsString                        mBuffer;   
    nsTArray<nsAutoPtr<convToken> > mTokens;   
    convToken                       *mToken;   
    nsString                        mPageTitle; 
    bool                            mPreFormatHTML; 
};

#endif 

