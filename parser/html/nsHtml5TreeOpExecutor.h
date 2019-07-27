



#ifndef nsHtml5TreeOpExecutor_h
#define nsHtml5TreeOpExecutor_h

#include "nsIAtom.h"
#include "nsTraceRefcnt.h"
#include "nsHtml5TreeOperation.h"
#include "nsHtml5SpeculativeLoad.h"
#include "nsTArray.h"
#include "nsContentSink.h"
#include "nsNodeInfoManager.h"
#include "nsHtml5DocumentMode.h"
#include "nsIScriptElement.h"
#include "nsIParser.h"
#include "nsAHtml5TreeOpSink.h"
#include "nsHtml5TreeOpStage.h"
#include "nsIURI.h"
#include "nsTHashtable.h"
#include "nsHashKeys.h"
#include "mozilla/LinkedList.h"
#include "nsHtml5DocumentBuilder.h"
#include "mozilla/net/ReferrerPolicy.h"

class nsHtml5Parser;
class nsHtml5StreamParser;
class nsIContent;
class nsIDocument;

class nsHtml5TreeOpExecutor final : public nsHtml5DocumentBuilder,
                                    public nsIContentSink,
                                    public nsAHtml5TreeOpSink,
                                    public mozilla::LinkedListElement<nsHtml5TreeOpExecutor>
{
  friend class nsHtml5FlushLoopGuard;
  typedef mozilla::net::ReferrerPolicy ReferrerPolicy;

  public:
    NS_DECL_AND_IMPL_ZEROING_OPERATOR_NEW
    NS_DECL_ISUPPORTS_INHERITED

  private:
    static bool        sExternalViewSource;
#ifdef DEBUG_NS_HTML5_TREE_OP_EXECUTOR_FLUSH
    static uint32_t    sAppendBatchMaxSize;
    static uint32_t    sAppendBatchSlotsExamined;
    static uint32_t    sAppendBatchExaminations;
    static uint32_t    sLongestTimeOffTheEventLoop;
    static uint32_t    sTimesFlushLoopInterrupted;
#endif

    


    bool                                 mSuppressEOF;
    
    bool                                 mReadingFromStage;
    nsTArray<nsHtml5TreeOperation>       mOpQueue;
    nsHtml5StreamParser*                 mStreamParser;
    
    


    nsTHashtable<nsCStringHashKey> mPreloadedURLs;

    nsCOMPtr<nsIURI> mSpeculationBaseURI;

    


    bool             mSpeculationReferrerPolicyWasSet;
    ReferrerPolicy   mSpeculationReferrerPolicy;

    nsCOMPtr<nsIURI> mViewSourceBaseURI;

    


    bool                          mStarted;

    nsHtml5TreeOpStage            mStage;

    bool                          mRunFlushLoopOnStack;

    bool                          mCallContinueInterruptedParsingIfEnabled;

    



    bool                          mAlreadyComplainedAboutCharset;

  public:

    nsHtml5TreeOpExecutor();

  protected:

    virtual ~nsHtml5TreeOpExecutor();

  public:

    

    


    NS_IMETHOD WillParse() override;

    


    NS_IMETHOD WillBuildModel(nsDTDMode aDTDMode) override;

    


    NS_IMETHOD DidBuildModel(bool aTerminated) override;

    


    NS_IMETHOD WillInterrupt() override;

    


    NS_IMETHOD WillResume() override;

    


    NS_IMETHOD SetParser(nsParserBase* aParser) override;

    


    virtual void FlushPendingNotifications(mozFlushType aType) override;

    


    NS_IMETHOD SetDocumentCharset(nsACString& aCharset) override {
    	NS_NOTREACHED("No one should call this.");
    	return NS_ERROR_NOT_IMPLEMENTED;
    }

    


    virtual nsISupports *GetTarget() override;
  
    virtual void ContinueInterruptedParsingAsync() override;

    bool IsScriptExecuting() override
    {
      return IsScriptExecutingImpl();
    }

    

    void SetStreamParser(nsHtml5StreamParser* aStreamParser)
    {
      mStreamParser = aStreamParser;
    }
    
    void InitializeDocWriteParserState(nsAHtml5TreeBuilderState* aState, int32_t aLine);

    bool IsScriptEnabled();

    virtual nsresult MarkAsBroken(nsresult aReason) override;

    void StartLayout();
    
    void FlushSpeculativeLoads();
                  
    void RunFlushLoop();

    nsresult FlushDocumentWrite();

    void MaybeSuspend();

    void Start();

    void NeedsCharsetSwitchTo(const char* aEncoding,
                              int32_t aSource,
                              uint32_t aLineNumber);

    void MaybeComplainAboutCharset(const char* aMsgId,
                                   bool aError,
                                   uint32_t aLineNumber);

    void ComplainAboutBogusProtocolCharset(nsIDocument* aDoc);

    bool IsComplete()
    {
      return !mParser;
    }
    
    bool HasStarted()
    {
      return mStarted;
    }
    
    bool IsFlushing()
    {
      return mFlushState >= eInFlush;
    }

#ifdef DEBUG
    bool IsInFlushLoop()
    {
      return mRunFlushLoopOnStack;
    }
#endif
    
    void RunScript(nsIContent* aScriptElement);
    
    



    virtual void MoveOpsFrom(nsTArray<nsHtml5TreeOperation>& aOpQueue) override;
    
    nsHtml5TreeOpStage* GetStage()
    {
      return &mStage;
    }
    
    void StartReadingFromStage()
    {
      mReadingFromStage = true;
    }

    void StreamEnded();
    
#ifdef DEBUG
    void AssertStageEmpty()
    {
      mStage.AssertEmpty();
    }
#endif

    nsIURI* GetViewSourceBaseURI();

    void PreloadScript(const nsAString& aURL,
                       const nsAString& aCharset,
                       const nsAString& aType,
                       const nsAString& aCrossOrigin,
                       bool aScriptFromHead);

    void PreloadStyle(const nsAString& aURL, const nsAString& aCharset,
                      const nsAString& aCrossOrigin);

    void PreloadImage(const nsAString& aURL,
                      const nsAString& aCrossOrigin,
                      const nsAString& aSrcset,
                      const nsAString& aSizes);

    void PreloadOpenPicture();

    void PreloadEndPicture();

    void PreloadPictureSource(const nsAString& aSrcset,
                              const nsAString& aSizes,
                              const nsAString& aType,
                              const nsAString& aMedia);

    void SetSpeculationBase(const nsAString& aURL);

    void SetSpeculationReferrerPolicy(ReferrerPolicy aReferrerPolicy);
    void SetSpeculationReferrerPolicy(const nsAString& aReferrerPolicy);

    void AddBase(const nsAString& aURL);

    static void InitializeStatics();

  private:
    nsHtml5Parser* GetParser();

    bool IsExternalViewSource();

    


    already_AddRefed<nsIURI> ConvertIfNotPreloadedYet(const nsAString& aURL);

    


    nsIURI* BaseURIForPreload();

    



    bool ShouldPreloadURI(nsIURI *aURI);
};

#endif 
