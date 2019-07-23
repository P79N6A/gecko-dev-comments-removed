










































#ifndef __CParserContext
#define __CParserContext

#include "nsIParser.h"
#include "nsIURL.h"
#include "nsIDTD.h"
#include "nsIStreamListener.h"
#include "nsIRequest.h"
#include "nsScanner.h"
#include "nsString.h"
#include "nsCOMPtr.h"
#include "nsAutoPtr.h"






class CParserContext {

public:

    enum {eTransferBufferSize=4096};
    enum eContextType {eCTNone,eCTURL,eCTString,eCTStream};

   CParserContext(  nsScanner* aScanner,
                    void* aKey=0, 
                    eParserCommands aCommand=eViewNormal,
                    nsIRequestObserver* aListener=0, 
                    nsIDTD *aDTD=0, 
                    eAutoDetectResult aStatus=eUnknownDetect, 
                    PRBool aCopyUnused=PR_FALSE); 
    
    ~CParserContext();

    nsresult GetTokenizer(PRInt32 aType,
                          nsIContentSink* aSink,
                          nsITokenizer*& aTokenizer);
    void  SetMimeType(const nsACString& aMimeType);

    nsCOMPtr<nsIRequest> mRequest; 
                                   
    nsCOMPtr<nsIDTD>	 mDTD;
    nsCOMPtr<nsIRequestObserver> mListener;
    nsAutoArrayPtr<char> mTransferBuffer;
    void*                mKey;
    nsCOMPtr<nsITokenizer> mTokenizer;
    CParserContext*      mPrevContext;
    nsAutoPtr<nsScanner> mScanner;
    
    nsCString            mMimeType;
    nsDTDMode            mDTDMode;
    
    eParserDocType       mDocType;
    eStreamState         mStreamListenerState; 
    eContextType         mContextType;
    eAutoDetectResult    mAutoDetectStatus;
    eParserCommands      mParserCommand;   

    PRPackedBool         mMultipart;
    PRPackedBool         mCopyUnused;
    PRUint32             mTransferBufferSize;
};

#endif
