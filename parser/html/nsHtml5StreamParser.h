





































#ifndef nsHtml5StreamParser_h__
#define nsHtml5StreamParser_h__

#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsIStreamListener.h"
#include "nsICharsetDetectionObserver.h"
#include "nsHtml5MetaScanner.h"
#include "nsIUnicodeDecoder.h"
#include "nsHtml5TreeOpExecutor.h"
#include "nsHtml5UTF16Buffer.h"
#include "nsIInputStream.h"
#include "nsICharsetAlias.h"

class nsHtml5Parser;

#define NS_HTML5_STREAM_PARSER_READ_BUFFER_SIZE 1024
#define NS_HTML5_STREAM_PARSER_SNIFFING_BUFFER_SIZE 512

enum eBomState {
  


  BOM_SNIFFING_NOT_STARTED = 0,

  



  SEEN_UTF_16_LE_FIRST_BYTE = 1,

  



  SEEN_UTF_16_BE_FIRST_BYTE = 2,

  



  SEEN_UTF_8_FIRST_BYTE = 3,

  



  SEEN_UTF_8_SECOND_BYTE = 4,

  


  BOM_SNIFFING_OVER = 5
};

class nsHtml5StreamParser : public nsIStreamListener,
                            public nsICharsetDetectionObserver {
  public:
    NS_DECL_AND_IMPL_ZEROING_OPERATOR_NEW
    NS_DECL_CYCLE_COLLECTING_ISUPPORTS
    NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(nsHtml5StreamParser, nsIStreamListener)

    nsHtml5StreamParser(nsHtml5Tokenizer* aTokenizer,
                        nsHtml5TreeOpExecutor* aExecutor,
                        nsHtml5Parser* aOwner);
                        
    virtual ~nsHtml5StreamParser();

    
    NS_DECL_NSIREQUESTOBSERVER
    
    NS_DECL_NSISTREAMLISTENER
    
    
    


    NS_IMETHOD Notify(const char* aCharset, nsDetectionConfident aConf);

    
    


    void internalEncodingDeclaration(nsString* aEncoding);

    

    






    inline void SetDocumentCharset(const nsACString& aCharset, PRInt32 aSource) {
      mCharset = aCharset;
      mCharsetSource = aSource;
    }
    
    inline void SetObserver(nsIRequestObserver* aObserver) {
      mObserver = aObserver;
    }
    
    nsresult GetChannel(nsIChannel** aChannel);

    inline void Block() {
      mBlocked = PR_TRUE;
    }
    
    inline void Unblock() {
      mBlocked = PR_FALSE;
    }

    inline void Suspend() {
      mSuspending = PR_TRUE;
    }

    void ParseUntilSuspend();
    
    PRBool IsDone() {
      return mDone;
    }
    
  private:

    static NS_METHOD ParserWriteFunc(nsIInputStream* aInStream,
                                     void* aHtml5StreamParser,
                                     const char* aFromSegment,
                                     PRUint32 aToOffset,
                                     PRUint32 aCount,
                                     PRUint32* aWriteCount);

    


    inline PRBool HasDecoder() {
      return !!mUnicodeDecoder;
    }

    


    nsresult SniffStreamBytes(const PRUint8* aFromSegment,
                              PRUint32 aCount,
                              PRUint32* aWriteCount);

    


    nsresult WriteStreamBytes(const PRUint8* aFromSegment,
                              PRUint32 aCount,
                              PRUint32* aWriteCount);
    
    












    nsresult FinalizeSniffing(const PRUint8* aFromSegment,
                              PRUint32 aCount,
                              PRUint32* aWriteCount,
                              PRUint32 aCountToSniffingLimit);

    










    nsresult SetupDecodingAndWriteSniffingBufferAndCurrentSegment(const PRUint8* aFromSegment,
                                                                  PRUint32 aCount,
                                                                  PRUint32* aWriteCount);

    










    nsresult WriteSniffingBufferAndCurrentSegment(const PRUint8* aFromSegment,
                                                  PRUint32 aCount,
                                                  PRUint32* aWriteCount);

    









    nsresult SetupDecodingFromBom(const char* aCharsetName,
                                  const char* aDecoderCharsetName);

    nsCOMPtr<nsIRequest>          mRequest;
    nsCOMPtr<nsIRequestObserver>  mObserver;

    


    nsCOMPtr<nsIUnicodeDecoder>   mUnicodeDecoder;

    


    nsAutoArrayPtr<PRUint8>       mSniffingBuffer;

    


    PRUint32                      mSniffingLength;

    


    eBomState                     mBomState;

    


    nsAutoPtr<nsHtml5MetaScanner> mMetaScanner;

    
    


    PRInt32                       mCharsetSource;

    


    nsCString                     mCharset;

    
    


    nsHtml5UTF16Buffer*           mFirstBuffer; 

    


    nsHtml5UTF16Buffer*           mLastBuffer; 
                      

    


    nsHtml5TreeOpExecutor*        mExecutor;

    


    nsHtml5TreeBuilder*           mTreeBuilder;

    


    nsHtml5Tokenizer*             mTokenizer;

    nsCOMPtr<nsHtml5Parser>       mOwner;

    


    PRBool                        mLastWasCR;

    


    PRBool                        mBlocked;

    


    PRBool                        mSuspending;

    


    PRBool                        mDone;

#ifdef DEBUG
    


    eStreamState                  mStreamListenerState;
#endif

};

#endif 
