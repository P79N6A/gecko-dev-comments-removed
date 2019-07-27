




#ifndef nsHtml5StreamParser_h
#define nsHtml5StreamParser_h

#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsICharsetDetectionObserver.h"
#include "nsHtml5MetaScanner.h"
#include "nsIUnicodeDecoder.h"
#include "nsHtml5TreeOpExecutor.h"
#include "nsHtml5OwningUTF16Buffer.h"
#include "nsIInputStream.h"
#include "mozilla/Mutex.h"
#include "nsHtml5AtomTable.h"
#include "nsHtml5Speculation.h"
#include "nsITimer.h"
#include "nsICharsetDetector.h"

class nsHtml5Parser;

#define NS_HTML5_STREAM_PARSER_READ_BUFFER_SIZE 1024
#define NS_HTML5_STREAM_PARSER_SNIFFING_BUFFER_SIZE 1024

enum eParserMode {
  


  NORMAL,

  


  VIEW_SOURCE_HTML,

  


  VIEW_SOURCE_XML,

  


  VIEW_SOURCE_PLAIN,

  


  PLAIN_TEXT,

  


  LOAD_AS_DATA
};

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

class nsHtml5StreamParser : public nsICharsetDetectionObserver {

  friend class nsHtml5RequestStopper;
  friend class nsHtml5DataAvailable;
  friend class nsHtml5StreamParserContinuation;
  friend class nsHtml5TimerKungFu;

  public:
    NS_DECL_AND_IMPL_ZEROING_OPERATOR_NEW
    NS_DECL_CYCLE_COLLECTING_ISUPPORTS
    NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(nsHtml5StreamParser,
                                             nsICharsetDetectionObserver)

    static void InitializeStatics();

    nsHtml5StreamParser(nsHtml5TreeOpExecutor* aExecutor,
                        nsHtml5Parser* aOwner,
                        eParserMode aMode);

    
    nsresult CheckListenerChain();

    nsresult OnStartRequest(nsIRequest* aRequest, nsISupports* aContext);

    nsresult OnDataAvailable(nsIRequest* aRequest,
                             nsISupports* aContext,
                             nsIInputStream* aInStream,
                             uint64_t aSourceOffset,
                             uint32_t aLength);

    nsresult OnStopRequest(nsIRequest* aRequest,
                           nsISupports* aContext,
                           nsresult status);

    
    


    NS_IMETHOD Notify(const char* aCharset, nsDetectionConfident aConf) override;

    
    
    


    bool internalEncodingDeclaration(nsString* aEncoding);

    

    






    inline void SetDocumentCharset(const nsACString& aCharset, int32_t aSource) {
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

    nsresult GetChannel(nsIChannel** aChannel);

    




    void ContinueAfterScripts(nsHtml5Tokenizer* aTokenizer, 
                              nsHtml5TreeBuilder* aTreeBuilder,
                              bool aLastWasCR);

    


    void ContinueAfterFailedCharsetSwitch();

    void Terminate()
    {
      mozilla::MutexAutoLock autoLock(mTerminatedMutex);
      mTerminated = true;
    }
    
    void DropTimer();

    




    void SetEncodingFromExpat(const char16_t* aEncoding);

    




    void SetViewSourceTitle(nsIURI* aURL);

  private:
    virtual ~nsHtml5StreamParser();

#ifdef DEBUG
    bool IsParserThread() {
      bool ret;
      mThread->IsOnCurrentThread(&ret);
      return ret;
    }
#endif

    void MarkAsBroken(nsresult aRv);

    





    void Interrupt()
    {
      mozilla::MutexAutoLock autoLock(mTerminatedMutex);
      mInterrupted = true;
    }

    void Uninterrupt()
    {
      NS_ASSERTION(IsParserThread(), "Wrong thread!");
      mTokenizerMutex.AssertCurrentThreadOwns();
      
      
      mInterrupted = false;      
    }

    



    void FlushTreeOpsAndDisarmTimer();

    void ParseAvailableData();
    
    void DoStopRequest();
    
    void DoDataAvailable(const uint8_t* aBuffer, uint32_t aLength);

    static NS_METHOD CopySegmentsToParser(nsIInputStream *aInStream,
                                          void *aClosure,
                                          const char *aFromSegment,
                                          uint32_t aToOffset,
                                          uint32_t aCount,
                                          uint32_t *aWriteCount);

    bool IsTerminatedOrInterrupted()
    {
      mozilla::MutexAutoLock autoLock(mTerminatedMutex);
      return mTerminated || mInterrupted;
    }

    bool IsTerminated()
    {
      mozilla::MutexAutoLock autoLock(mTerminatedMutex);
      return mTerminated;
    }

    


    inline bool HasDecoder()
    {
      return !!mUnicodeDecoder;
    }

    


    nsresult SniffStreamBytes(const uint8_t* aFromSegment,
                              uint32_t aCount,
                              uint32_t* aWriteCount);

    


    nsresult WriteStreamBytes(const uint8_t* aFromSegment,
                              uint32_t aCount,
                              uint32_t* aWriteCount);

    


    void SniffBOMlessUTF16BasicLatin(const uint8_t* aFromSegment,
                                     uint32_t aCountToSniffingLimit);

    












    nsresult FinalizeSniffing(const uint8_t* aFromSegment,
                              uint32_t aCount,
                              uint32_t* aWriteCount,
                              uint32_t aCountToSniffingLimit);

    










    nsresult SetupDecodingAndWriteSniffingBufferAndCurrentSegment(const uint8_t* aFromSegment,
                                                                  uint32_t aCount,
                                                                  uint32_t* aWriteCount);

    







    nsresult SetupDecodingFromBom(const char* aDecoderCharsetName);

    







    bool PreferredForInternalEncodingDecl(nsACString& aEncoding);

    


    static void TimerCallback(nsITimer* aTimer, void* aClosure);

    



    void TimerFlush();

    


    void MaybeDisableFutureSpeculation()
    {
        mSpeculationFailureCount++;
    }

    





    bool IsSpeculationEnabled()
    {
        return mSpeculationFailureCount < 100;
    }

    nsCOMPtr<nsIRequest>          mRequest;
    nsCOMPtr<nsIRequestObserver>  mObserver;

    


    nsCString                     mViewSourceTitle;

    


    nsCOMPtr<nsIUnicodeDecoder>   mUnicodeDecoder;

    


    nsAutoArrayPtr<uint8_t>       mSniffingBuffer;

    


    uint32_t                      mSniffingLength;

    


    eBomState                     mBomState;

    


    nsAutoPtr<nsHtml5MetaScanner> mMetaScanner;

    
    


    int32_t                       mCharsetSource;

    


    nsCString                     mCharset;

    


    bool                          mReparseForbidden;

    
    


    nsRefPtr<nsHtml5OwningUTF16Buffer> mFirstBuffer;

    


    nsHtml5OwningUTF16Buffer*     mLastBuffer; 
                      

    


    nsHtml5TreeOpExecutor*        mExecutor;

    


    nsAutoPtr<nsHtml5TreeBuilder> mTreeBuilder;

    


    nsAutoPtr<nsHtml5Tokenizer>   mTokenizer;

    



    mozilla::Mutex                mTokenizerMutex;

    


    nsHtml5AtomTable              mAtomTable;

    


    nsRefPtr<nsHtml5Parser>       mOwner;

    


    bool                          mLastWasCR;

    


    eHtml5StreamState             mStreamState;
    
    


    bool                          mSpeculating;

    


    bool                          mAtEOF;

    





    nsTArray<nsAutoPtr<nsHtml5Speculation> >  mSpeculations;
    mozilla::Mutex                            mSpeculationMutex;

    


    uint32_t                      mSpeculationFailureCount;

    


    bool                          mTerminated;
    bool                          mInterrupted;
    mozilla::Mutex                mTerminatedMutex;
    
    


    nsCOMPtr<nsIThread>           mThread;
    
    nsCOMPtr<nsIRunnable>         mExecutorFlusher;
    
    nsCOMPtr<nsIRunnable>         mLoadFlusher;

    


    nsCOMPtr<nsICharsetDetector>  mChardet;

    


    bool                          mFeedChardet;

    


    bool                          mInitialEncodingWasFromParentFrame;

    


    nsCOMPtr<nsITimer>            mFlushTimer;

    



    bool                          mFlushTimerArmed;

    


    bool                          mFlushTimerEverFired;

    


    eParserMode                   mMode;

    




    static int32_t                sTimerInitialDelay;

    




    static int32_t                sTimerSubsequentDelay;
};

#endif 
