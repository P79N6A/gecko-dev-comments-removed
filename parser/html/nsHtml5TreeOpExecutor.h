




































#ifndef nsHtml5TreeOpExecutor_h__
#define nsHtml5TreeOpExecutor_h__

#include "prtypes.h"
#include "nsIAtom.h"
#include "nsINameSpaceManager.h"
#include "nsIContent.h"
#include "nsIDocument.h"
#include "nsTraceRefcnt.h"
#include "nsHtml5TreeOperation.h"
#include "nsHtml5SpeculativeLoad.h"
#include "nsHtml5PendingNotification.h"
#include "nsTArray.h"
#include "nsContentSink.h"
#include "nsNodeInfoManager.h"
#include "nsHtml5DocumentMode.h"
#include "nsIScriptElement.h"
#include "nsIParser.h"
#include "nsCOMArray.h"
#include "nsAHtml5TreeOpSink.h"
#include "nsHtml5TreeOpStage.h"
#include "nsHashSets.h"
#include "nsIURI.h"

class nsHtml5TreeBuilder;
class nsHtml5Tokenizer;
class nsHtml5StreamParser;

typedef nsIContent* nsIContentPtr;

enum eHtml5FlushState {
  eNotFlushing = 0,  
  eInFlush = 1,      
  eInDocUpdate = 2,  
  eNotifying = 3     
};

class nsHtml5TreeOpExecutor : public nsContentSink,
                              public nsIContentSink,
                              public nsAHtml5TreeOpSink
{
  friend class nsHtml5FlushLoopGuard;

  public:
    NS_DECL_AND_IMPL_ZEROING_OPERATOR_NEW
    NS_DECL_ISUPPORTS_INHERITED
    NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(nsHtml5TreeOpExecutor, nsContentSink)

  private:
#ifdef DEBUG_NS_HTML5_TREE_OP_EXECUTOR_FLUSH
    static PRUint32    sAppendBatchMaxSize;
    static PRUint32    sAppendBatchSlotsExamined;
    static PRUint32    sAppendBatchExaminations;
    static PRUint32    sLongestTimeOffTheEventLoop;
    static PRUint32    sTimesFlushLoopInterrupted;
#endif

    


    PRBool                               mSuppressEOF;
    
    PRBool                               mReadingFromStage;
    nsTArray<nsHtml5TreeOperation>       mOpQueue;
    nsTArray<nsIContentPtr>              mElementsSeenInThisAppendBatch;
    nsTArray<nsHtml5PendingNotification> mPendingNotifications;
    nsHtml5StreamParser*                 mStreamParser;
    nsCOMArray<nsIContent>               mOwnedElements;
    
    


    nsCStringHashSet mPreloadedURLs;

    nsCOMPtr<nsIURI> mSpeculationBaseURI;

    


    PRBool                        mStarted;

    nsHtml5TreeOpStage            mStage;

    eHtml5FlushState              mFlushState;

    PRBool                        mRunFlushLoopOnStack;

    PRBool                        mCallContinueInterruptedParsingIfEnabled;

    PRBool                        mPreventScriptExecution;

  public:
  
    nsHtml5TreeOpExecutor();
    virtual ~nsHtml5TreeOpExecutor();
  
    

    


    NS_IMETHOD WillParse();

    


    NS_IMETHOD WillBuildModel(nsDTDMode aDTDMode) {
      NS_ASSERTION(GetDocument()->GetScriptGlobalObject(), 
                   "Script global object not ready");
      mDocument->AddObserver(this);
      WillBuildModelImpl();
      GetDocument()->BeginLoad();
      return NS_OK;
    }

    


    NS_IMETHOD DidBuildModel(PRBool aTerminated);

    


    NS_IMETHOD WillInterrupt();

    


    NS_IMETHOD WillResume();

    


    NS_IMETHOD SetParser(nsIParser* aParser);

    


    virtual void FlushPendingNotifications(mozFlushType aType);

    


    NS_IMETHOD SetDocumentCharset(nsACString& aCharset) {
    	NS_NOTREACHED("No one should call this.");
    	return NS_ERROR_NOT_IMPLEMENTED;
    }

    


    virtual nsISupports *GetTarget();

    virtual void ScrollToRef() { ScrollToRefImpl(); }

    
    virtual void UpdateChildCounts();
    virtual nsresult FlushTags();
    virtual void PostEvaluateScript(nsIScriptElement *aElement);
    virtual void ContinueInterruptedParsingAsync();
 
    


    void UpdateStyleSheet(nsIContent* aElement);

    
    nsIDocument* GetDocument() {
      return mDocument;
    }
    nsNodeInfoManager* GetNodeInfoManager() {
      return mNodeInfoManager;
    }
    nsIDocShell* GetDocShell() {
      return mDocShell;
    }

    PRBool IsScriptExecuting() {
      return IsScriptExecutingImpl();
    }
    
    void SetNodeInfoManager(nsNodeInfoManager* aManager) {
      mNodeInfoManager = aManager;
    }
    
    

    void SetDocumentCharsetAndSource(nsACString& aCharset, PRInt32 aCharsetSource);

    void SetStreamParser(nsHtml5StreamParser* aStreamParser) {
      mStreamParser = aStreamParser;
    }
    
    void InitializeDocWriteParserState(nsAHtml5TreeBuilderState* aState, PRInt32 aLine);

    PRBool IsScriptEnabled();

    






    void EnableFragmentMode(PRBool aPreventScriptExecution) {
      mFragmentMode = PR_TRUE;
      mPreventScriptExecution = aPreventScriptExecution;
    }
    
    PRBool IsFragmentMode() {
      return mFragmentMode;
    }

    inline void BeginDocUpdate() {
      NS_PRECONDITION(mFlushState == eInFlush, "Tried to double-open update.");
      NS_PRECONDITION(mParser, "Started update without parser.");
      mFlushState = eInDocUpdate;
      mDocument->BeginUpdate(UPDATE_CONTENT_MODEL);
    }

    inline void EndDocUpdate() {
      NS_PRECONDITION(mFlushState != eNotifying, "mFlushState out of sync");
      if (mFlushState == eInDocUpdate) {
        FlushPendingAppendNotifications();
        mFlushState = eInFlush;
        mDocument->EndUpdate(UPDATE_CONTENT_MODEL);
      }
    }

    void PostPendingAppendNotification(nsIContent* aParent, nsIContent* aChild) {
      PRBool newParent = PR_TRUE;
      const nsIContentPtr* first = mElementsSeenInThisAppendBatch.Elements();
      const nsIContentPtr* last = first + mElementsSeenInThisAppendBatch.Length() - 1;
      for (const nsIContentPtr* iter = last; iter >= first; --iter) {
#ifdef DEBUG_NS_HTML5_TREE_OP_EXECUTOR_FLUSH
        sAppendBatchSlotsExamined++;
#endif
        if (*iter == aParent) {
          newParent = PR_FALSE;
          break;
        }
      }
      if (aChild->IsElement()) {
        mElementsSeenInThisAppendBatch.AppendElement(aChild);
      }
      mElementsSeenInThisAppendBatch.AppendElement(aParent);
      if (newParent) {
        mPendingNotifications.AppendElement(aParent);
      }
#ifdef DEBUG_NS_HTML5_TREE_OP_EXECUTOR_FLUSH
      sAppendBatchExaminations++;
#endif
    }

    void FlushPendingAppendNotifications() {
      NS_PRECONDITION(mFlushState == eInDocUpdate, "Notifications flushed outside update");
      mFlushState = eNotifying;
      const nsHtml5PendingNotification* start = mPendingNotifications.Elements();
      const nsHtml5PendingNotification* end = start + mPendingNotifications.Length();
      for (nsHtml5PendingNotification* iter = (nsHtml5PendingNotification*)start; iter < end; ++iter) {
        iter->Fire();
      }
      mPendingNotifications.Clear();
#ifdef DEBUG_NS_HTML5_TREE_OP_EXECUTOR_FLUSH
      if (mElementsSeenInThisAppendBatch.Length() > sAppendBatchMaxSize) {
        sAppendBatchMaxSize = mElementsSeenInThisAppendBatch.Length();
      }
#endif
      mElementsSeenInThisAppendBatch.Clear();
      NS_ASSERTION(mFlushState == eNotifying, "mFlushState out of sync");
      mFlushState = eInDocUpdate;
    }
    
    inline PRBool HaveNotified(nsIContent* aNode) {
      NS_PRECONDITION(aNode, "HaveNotified called with null argument.");
      const nsHtml5PendingNotification* start = mPendingNotifications.Elements();
      const nsHtml5PendingNotification* end = start + mPendingNotifications.Length();
      for (;;) {
        nsIContent* parent = aNode->GetParent();
        if (!parent) {
          return PR_TRUE;
        }
        for (nsHtml5PendingNotification* iter = (nsHtml5PendingNotification*)start; iter < end; ++iter) {
          if (iter->Contains(parent)) {
            return iter->HaveNotifiedIndex(parent->IndexOf(aNode));
          }
        }
        aNode = parent;
      }
    }

    void StartLayout();
    
    void SetDocumentMode(nsHtml5DocumentMode m);

    nsresult Init(nsIDocument* aDoc, nsIURI* aURI,
                  nsISupports* aContainer, nsIChannel* aChannel);

    void FlushSpeculativeLoads();
                  
    void RunFlushLoop();

    void FlushDocumentWrite();

    void MaybeSuspend();

    void Start();

    void NeedsCharsetSwitchTo(const char* aEncoding, PRInt32 aSource);
    
    PRBool IsComplete() {
      return !mParser;
    }
    
    PRBool HasStarted() {
      return mStarted;
    }
    
    PRBool IsFlushing() {
      return mFlushState >= eInFlush;
    }

#ifdef DEBUG
    PRBool IsInFlushLoop() {
      return mRunFlushLoopOnStack;
    }
#endif
    
    void RunScript(nsIContent* aScriptElement);
    
    void Reset();
    
    inline void HoldElement(nsIContent* aContent) {
      mOwnedElements.AppendObject(aContent);
    }

    void DropHeldElements() {
      mOwnedElements.Clear();
    }

    



    virtual void MoveOpsFrom(nsTArray<nsHtml5TreeOperation>& aOpQueue);
    
    nsHtml5TreeOpStage* GetStage() {
      return &mStage;
    }
    
    void StartReadingFromStage() {
      mReadingFromStage = PR_TRUE;
    }

    void StreamEnded();
    
#ifdef DEBUG
    void AssertStageEmpty() {
      mStage.AssertEmpty();
    }
#endif

    void PreloadScript(const nsAString& aURL,
                       const nsAString& aCharset,
                       const nsAString& aType);

    void PreloadStyle(const nsAString& aURL, const nsAString& aCharset);

    void PreloadImage(const nsAString& aURL);

    void SetSpeculationBase(const nsAString& aURL);

  private:

    nsHtml5Tokenizer* GetTokenizer();

    


    already_AddRefed<nsIURI> ConvertIfNotPreloadedYet(const nsAString& aURL);

};

#endif 
