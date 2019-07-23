





































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
#include "mozilla/Mutex.h"
#include "nsHtml5AtomTable.h"
#include "nsHtml5Speculation.h"

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

enum eHtml5StreamState {
  STREAM_NOT_STARTED = 0,
  STREAM_BEING_READ = 1,
  STREAM_ENDED = 2
};

class nsHtml5StreamParser : public nsIStreamListener,
                            public nsICharsetDetectionObserver {

  friend class nsHtml5RequestStopper;
  friend class nsHtml5DataAvailable;
  friend class nsHtml5StreamParserContinuation;

  public:
    NS_DECL_AND_IMPL_ZEROING_OPERATOR_NEW
    NS_DECL_CYCLE_COLLECTING_ISUPPORTS
    NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(nsHtml5StreamParser, nsIStreamListener)

    nsHtml5StreamParser(nsHtml5TreeOpExecutor* aExecutor,
                        nsHtml5Parser* aOwner);
                        
    virtual ~nsHtml5StreamParser();

    
    NS_DECL_NSIREQUESTOBSERVER
    
    NS_DECL_NSISTREAMLISTENER
    
    
    


    NS_IMETHOD Notify(const char* aCharset, nsDetectionConfident aConf);

    
    


    void internalEncodingDeclaration(nsString* aEncoding);

    

    






    inline void SetDocumentCharset(const nsACString& aCharset, PRInt32 aSource) {
      NS_PRECONDITION(mStreamState == STREAM_NOT_STARTED,
                      "SetDocumentCharset called too late.");
      NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
      mCharset = aCharset;
      mCharsetSource = aSource;
    }
    
    inline void SetObserver(nsIRequestObserver* aObserver) {
      NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
      mObserver = aObserver;
    }

    void SetSpeculativeLoaderWithDocument(nsIDocument* aDocument);

    nsresult GetChannel(nsIChannel** aChannel);

    




    void ContinueAfterScripts(nsHtml5Tokenizer* aTokenizer, 
                              nsHtml5TreeBuilder* aTreeBuilder,
                              PRBool aLastWasCR);

    void Terminate() {
      mozilla::MutexAutoLock autoLock(mTerminatedMutex);
      mTerminated = PR_TRUE;
    }
    
  private:

#ifdef DEBUG
    PRBool IsParserThread() {
      PRBool ret;
      mThread->IsOnCurrentThread(&ret);
      return ret;
    }
#endif

    void Interrupt() {
      NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
      mozilla::MutexAutoLock autoLock(mTerminatedMutex);
      mInterrupted = PR_TRUE;
    }

    void Uninterrupt() {
      NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
      mTokenizerMutex.AssertCurrentThreadOwns();
      
      
      mInterrupted = PR_FALSE;      
    }

    void ParseAvailableData();
    
    void DoStopRequest();
    
    void DoDataAvailable(PRUint8* aBuffer, PRUint32 aLength);

    PRBool IsTerminatedOrInterrupted() {
      mozilla::MutexAutoLock autoLock(mTerminatedMutex);
      return mTerminated || mInterrupted;
    }

    PRBool IsTerminated() {
      mozilla::MutexAutoLock autoLock(mTerminatedMutex);
      return mTerminated;
    }

    


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

    


    PRBool                        mReparseForbidden;

    
    


    nsRefPtr<nsHtml5UTF16Buffer>  mFirstBuffer;

    


    nsHtml5UTF16Buffer*           mLastBuffer; 
                      

    


    nsHtml5TreeOpExecutor*        mExecutor;

    


    nsAutoPtr<nsHtml5TreeBuilder> mTreeBuilder;

    


    nsAutoPtr<nsHtml5Tokenizer>   mTokenizer;

    



    mozilla::Mutex                mTokenizerMutex;

    


    nsHtml5AtomTable              mAtomTable;

    


    nsCOMPtr<nsHtml5Parser>       mOwner;

    


    PRBool                        mLastWasCR;

    


    eHtml5StreamState             mStreamState;
    
    


    PRBool                        mSpeculating;

    


    PRBool                        mAtEOF;

    





    nsTArray<nsAutoPtr<nsHtml5Speculation> >  mSpeculations;
    mozilla::Mutex                            mSpeculationMutex;

    


    PRBool                        mTerminated;
    PRBool                        mInterrupted;
    mozilla::Mutex                mTerminatedMutex;
    
    


    nsCOMPtr<nsIThread>           mThread;
    
    nsCOMPtr<nsIRunnable>         mExecutorFlusher;
    
    


    nsCOMPtr<nsIDocument>         mDocument;
};

#endif 
