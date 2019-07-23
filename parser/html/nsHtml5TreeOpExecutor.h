




































#ifndef nsHtml5TreeOpExecutor_h__
#define nsHtml5TreeOpExecutor_h__

#include "prtypes.h"
#include "nsIAtom.h"
#include "nsINameSpaceManager.h"
#include "nsIContent.h"
#include "nsIDocument.h"
#include "nsTraceRefcnt.h"
#include "nsHtml5TreeOperation.h"
#include "nsHtml5PendingNotification.h"
#include "nsTArray.h"
#include "nsContentSink.h"
#include "nsNodeInfoManager.h"
#include "nsHtml5DocumentMode.h"
#include "nsITimer.h"
#include "nsIScriptElement.h"
#include "nsIParser.h"

class nsHtml5TreeBuilder;
class nsHtml5Tokenizer;
class nsHtml5StreamParser;

enum eHtml5ParserLifecycle {
  


  NOT_STARTED = 0,

  


  PARSING = 1,

  



  STREAM_ENDING = 2,

  


  TERMINATED = 3
};

typedef nsIContent* nsIContentPtr;

class nsHtml5TreeOpExecutor : public nsIContentSink,
                              public nsContentSink
{
  public:
    NS_DECL_AND_IMPL_ZEROING_OPERATOR_NEW
    NS_DECL_ISUPPORTS_INHERITED
    NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(nsHtml5TreeOpExecutor, nsContentSink)

  private:
#ifdef DEBUG_hsivonen
    static PRUint32    sInsertionBatchMaxLength;
    static PRUint32    sAppendBatchMaxSize;
    static PRUint32    sAppendBatchSlotsExamined;
    static PRUint32    sAppendBatchExaminations;
#endif
    static PRUint32                      sTreeOpQueueMaxLength;

    


    PRBool                               mSuppressEOF;
    
    PRBool                               mHasProcessedBase;
    PRBool                               mNeedsFlush;
    nsCOMPtr<nsITimer>                   mFlushTimer;
    nsTArray<nsHtml5TreeOperation>       mOpQueue;
    nsTArray<nsIContentPtr>              mElementsSeenInThisAppendBatch;
    nsTArray<nsHtml5PendingNotification> mPendingNotifications;
    nsHtml5StreamParser*                 mStreamParser;

    


    nsCString                     mPendingCharset;

    


    PRBool                        mNeedsCharsetSwitch;
  
    


    eHtml5ParserLifecycle         mLifeCycle;

    


    nsCOMPtr<nsIContent>          mScriptElement;
    
    nsHtml5TreeBuilder*           mTreeBuilder;

  public:
  
    nsHtml5TreeOpExecutor();
    virtual ~nsHtml5TreeOpExecutor();
  
    

    


    NS_IMETHOD WillParse();

    


    NS_IMETHOD WillBuildModel(nsDTDMode aDTDMode) {
      NS_ASSERTION(GetDocument()->GetScriptGlobalObject(), 
                   "Script global object not ready");
      WillBuildModelImpl();
      GetDocument()->BeginLoad();
      return NS_OK;
    }

    


    NS_IMETHOD DidBuildModel(PRBool aTerminated);

    


    NS_IMETHOD WillInterrupt();

    


    NS_IMETHOD WillResume();

    


    NS_IMETHOD SetParser(nsIParser* aParser);

    


    virtual void FlushPendingNotifications(mozFlushType aType);

    


    NS_IMETHOD SetDocumentCharset(nsACString& aCharset);

    


    virtual nsISupports *GetTarget();
  
    
    virtual nsresult ProcessBASETag(nsIContent* aContent);
    virtual void UpdateChildCounts();
    virtual nsresult FlushTags();
    virtual void PostEvaluateScript(nsIScriptElement *aElement);
    


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
    
    void AllowInterrupts() {
      mCanInterruptParser = PR_TRUE;
    }

    void ProhibitInterrupts() {
      mCanInterruptParser = PR_FALSE;
    }
        
    void SetBaseUriFromDocument() {
      mDocumentBaseURI = mDocument->GetBaseURI();
      mHasProcessedBase = PR_TRUE;
    }
    
    void SetNodeInfoManager(nsNodeInfoManager* aManager) {
      mNodeInfoManager = aManager;
    }
    
    void SetStreamParser(nsHtml5StreamParser* aStreamParser) {
      mStreamParser = aStreamParser;
    }
    
    


    nsresult MaybePerformCharsetSwitch();

    


    void ExecuteScript();
    
    PRBool IsScriptEnabled();

    void PostPendingAppendNotification(nsIContent* aParent, nsIContent* aChild) {
      PRBool newParent = PR_TRUE;
      const nsIContentPtr* first = mElementsSeenInThisAppendBatch.Elements();
      const nsIContentPtr* last = first + mElementsSeenInThisAppendBatch.Length() - 1;
      for (const nsIContentPtr* iter = last; iter >= first; --iter) {
#ifdef DEBUG_hsivonen
        sAppendBatchSlotsExamined++;
#endif
        if (*iter == aParent) {
          newParent = PR_FALSE;
          break;
        }
      }
      if (aChild->IsNodeOfType(nsINode::eELEMENT)) {
        mElementsSeenInThisAppendBatch.AppendElement(aChild);
      }
      mElementsSeenInThisAppendBatch.AppendElement(aParent);
      if (newParent) {
        mPendingNotifications.AppendElement(aParent);
      }
#ifdef DEBUG_hsivonen
      sAppendBatchExaminations++;
#endif
    }

    void FlushPendingAppendNotifications() {
      const nsHtml5PendingNotification* start = mPendingNotifications.Elements();
      const nsHtml5PendingNotification* end = start + mPendingNotifications.Length();
      for (nsHtml5PendingNotification* iter = (nsHtml5PendingNotification*)start; iter < end; ++iter) {
        iter->Fire();
      }
      mPendingNotifications.Clear();
#ifdef DEBUG_hsivonen
      if (mElementsSeenInThisAppendBatch.Length() > sAppendBatchMaxSize) {
        sAppendBatchMaxSize = mElementsSeenInThisAppendBatch.Length();
      }
#endif
      mElementsSeenInThisAppendBatch.Clear();
    }

    void StartLayout() {
      nsIDocument* doc = GetDocument();
      if (doc) {
        FlushPendingAppendNotifications();
        nsContentSink::StartLayout(PR_FALSE);
      }
    }
    
    void DocumentMode(nsHtml5DocumentMode m);

    nsresult Init(nsIDocument* aDoc, nsIURI* aURI,
                  nsISupports* aContainer, nsIChannel* aChannel);
                  
    void Flush();

    void MaybeSuspend();

    void MaybeFlush() {
      if (mNeedsFlush) {
        Flush();
      }
    }

    void DeferredTimerFlush();

    void Start();

    void End();
    
    void NeedsCharsetSwitchTo(const nsACString& aEncoding);
    
    void IgnoreCharsetSwitch() {
      mNeedsCharsetSwitch = PR_FALSE;
    }
    
#ifdef DEBUG
    PRBool NeedsCharsetSwitch() {
      return mNeedsCharsetSwitch;
    }
    
    PRBool HasScriptElement() {
      return !!mScriptElement;
    }
#endif

    PRBool IsComplete() {
      return (mLifeCycle == TERMINATED);
    }
    
    eHtml5ParserLifecycle GetLifeCycle() {
      return mLifeCycle;
    }
    
    void SetLifeCycle(eHtml5ParserLifecycle aLifeCycle) {
      mLifeCycle = aLifeCycle;
    }
    
    void MaybeExecuteScript();
    
    void MaybePreventExecution() {
      if (mScriptElement) {
        nsCOMPtr<nsIScriptElement> script = do_QueryInterface(mScriptElement);
        NS_ASSERTION(script, "mScriptElement didn't QI to nsIScriptElement!");
        script->PreventExecution();
        mScriptElement = nsnull;
      }    
    }
    
    


    void SetScriptElement(nsIContent* aScript) { 
      mScriptElement = aScript;
    }
    
    void SetTreeBuilder(nsHtml5TreeBuilder* aBuilder) {
      mTreeBuilder = aBuilder;
    }
    
    void Reset();

  private:

    nsHtml5Tokenizer* GetTokenizer();
    
    void FillQueue();
    
};

#endif 
