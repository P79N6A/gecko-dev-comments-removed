










































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
   enum eContextType {eCTNone,eCTURL,eCTString,eCTStream};

   CParserContext(CParserContext* aPrevContext,
                  nsScanner* aScanner,
                  void* aKey = 0,
                  eParserCommands aCommand = eViewNormal,
                  nsIRequestObserver* aListener = 0,
                  eAutoDetectResult aStatus = eUnknownDetect,
                  PRBool aCopyUnused = PR_FALSE);

    ~CParserContext();

    nsresult GetTokenizer(nsIDTD* aDTD,
                          nsIContentSink* aSink,
                          nsITokenizer*& aTokenizer);
    void  SetMimeType(const nsACString& aMimeType);

    nsCOMPtr<nsIRequest> mRequest; 
                                   
    nsCOMPtr<nsIRequestObserver> mListener;
    void* const          mKey;
    nsCOMPtr<nsITokenizer> mTokenizer;
    CParserContext* const mPrevContext;
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

    PRUint32             mNumConsumed;
};

#endif
