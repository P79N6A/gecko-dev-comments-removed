







































#include "nsHtml5TreeOpExecutor.h"
#include "nsScriptLoader.h"
#include "nsIMarkupDocumentViewer.h"
#include "nsIContentViewer.h"
#include "nsIDocShellTreeItem.h"
#include "nsIStyleSheetLinkingElement.h"
#include "nsStyleLinkElement.h"
#include "nsIDocShell.h"
#include "nsIScriptGlobalObject.h"
#include "nsIScriptGlobalObjectOwner.h"
#include "nsIScriptSecurityManager.h"
#include "nsIWebShellServices.h"
#include "nsContentUtils.h"
#include "mozAutoDocUpdate.h"
#include "nsNetUtil.h"
#include "nsHtml5Parser.h"
#include "nsHtml5Tokenizer.h"
#include "nsHtml5TreeBuilder.h"
#include "nsHtml5StreamParser.h"

NS_IMPL_CYCLE_COLLECTION_CLASS(nsHtml5TreeOpExecutor)

NS_INTERFACE_TABLE_HEAD_CYCLE_COLLECTION_INHERITED(nsHtml5TreeOpExecutor)
  NS_INTERFACE_TABLE_INHERITED1(nsHtml5TreeOpExecutor, 
                                nsIContentSink)
NS_INTERFACE_TABLE_TAIL_INHERITING(nsContentSink)

NS_IMPL_ADDREF_INHERITED(nsHtml5TreeOpExecutor, nsContentSink)

NS_IMPL_RELEASE_INHERITED(nsHtml5TreeOpExecutor, nsContentSink)

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(nsHtml5TreeOpExecutor, nsContentSink)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMARRAY(mOwnedElements)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(nsHtml5TreeOpExecutor, nsContentSink)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMARRAY(mOwnedElements)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

PRInt32 nsHtml5TreeOpExecutor::sTreeOpQueueLengthLimit = 200;
PRInt32 nsHtml5TreeOpExecutor::sTreeOpQueueMaxTime = 100; 
PRInt32 nsHtml5TreeOpExecutor::sTreeOpQueueMinLength = 100;
PRInt32 nsHtml5TreeOpExecutor::sTreeOpQueueMaxLength = 4500;


void
nsHtml5TreeOpExecutor::InitializeStatics()
{
  
  
  nsContentUtils::AddIntPrefVarCache("html5.opqueue.initiallengthlimit", 
                                     &sTreeOpQueueLengthLimit);
  nsContentUtils::AddIntPrefVarCache("html5.opqueue.maxtime", 
                                     &sTreeOpQueueMaxTime);
  nsContentUtils::AddIntPrefVarCache("html5.opqueue.minlength",
                                     &sTreeOpQueueMinLength);
  nsContentUtils::AddIntPrefVarCache("html5.opqueue.maxlength",
                                     &sTreeOpQueueMaxLength);
  
  
  if (sTreeOpQueueMinLength <= 0) {
    sTreeOpQueueMinLength = 200;
  }
  if (sTreeOpQueueLengthLimit < sTreeOpQueueMinLength) {
    sTreeOpQueueLengthLimit = sTreeOpQueueMinLength;
  }
  if (sTreeOpQueueMaxLength < sTreeOpQueueMinLength) {
    sTreeOpQueueMaxLength = sTreeOpQueueMinLength;
  }
  if (sTreeOpQueueMaxTime <= 0) {
    sTreeOpQueueMaxTime = 200;
  }
}

nsHtml5TreeOpExecutor::nsHtml5TreeOpExecutor()
{
  
}

nsHtml5TreeOpExecutor::~nsHtml5TreeOpExecutor()
{
  NS_ASSERTION(mOpQueue.IsEmpty(), "Somehow there's stuff in the op queue.");
}


NS_IMETHODIMP
nsHtml5TreeOpExecutor::WillParse()
{
  NS_NOTREACHED("No one should call this");
  return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
nsHtml5TreeOpExecutor::DidBuildModel(PRBool aTerminated)
{
  NS_PRECONDITION(mStarted, "Bad life cycle.");

  if (!aTerminated) {
    
    
    EndDocUpdate();
    
    
    
    if (!mParser) {
      return NS_OK;
    }
  }
  
  static_cast<nsHtml5Parser*> (mParser.get())->DropStreamParser();

  
  DidBuildModelImpl(aTerminated);
  mDocument->ScriptLoader()->RemoveObserver(this);
  ScrollToRef();
  mDocument->RemoveObserver(this);
  if (!mParser) {
    
    
    return NS_OK;
  }
  mDocument->EndLoad();
  DropParserAndPerfHint();
#ifdef GATHER_DOCWRITE_STATISTICS
  printf("UNSAFE SCRIPTS: %d\n", sUnsafeDocWrites);
  printf("TOKENIZER-SAFE SCRIPTS: %d\n", sTokenSafeDocWrites);
  printf("TREEBUILDER-SAFE SCRIPTS: %d\n", sTreeSafeDocWrites);
#endif
#ifdef DEBUG_NS_HTML5_TREE_OP_EXECUTOR_FLUSH
  printf("MAX NOTIFICATION BATCH LEN: %d\n", sAppendBatchMaxSize);
  if (sAppendBatchExaminations != 0) {
    printf("AVERAGE SLOTS EXAMINED: %d\n", sAppendBatchSlotsExamined / sAppendBatchExaminations);
  }
#endif
  return NS_OK;
}

NS_IMETHODIMP
nsHtml5TreeOpExecutor::WillInterrupt()
{
  return WillInterruptImpl();
}

NS_IMETHODIMP
nsHtml5TreeOpExecutor::WillResume()
{
  WillResumeImpl();
  return WillParseImpl();
}

NS_IMETHODIMP
nsHtml5TreeOpExecutor::SetParser(nsIParser* aParser)
{
  mParser = aParser;
  return NS_OK;
}

void
nsHtml5TreeOpExecutor::FlushPendingNotifications(mozFlushType aType)
{
}

void
nsHtml5TreeOpExecutor::SetDocumentCharsetAndSource(nsACString& aCharset, PRInt32 aCharsetSource)
{
  if (mDocument) {
    mDocument->SetDocumentCharacterSetSource(aCharsetSource);
    mDocument->SetDocumentCharacterSet(aCharset);
  }
  if (mDocShell) {
    
    
    
    
    nsCOMPtr<nsIMarkupDocumentViewer> muCV;
    nsCOMPtr<nsIContentViewer> cv;
    mDocShell->GetContentViewer(getter_AddRefs(cv));
    if (cv) {
      muCV = do_QueryInterface(cv);
    } else {
      
      
      
      nsCOMPtr<nsIDocShellTreeItem> docShellAsItem =
        do_QueryInterface(mDocShell);
      if (!docShellAsItem) {
    	  return;
      }
      nsCOMPtr<nsIDocShellTreeItem> parentAsItem;
      docShellAsItem->GetSameTypeParent(getter_AddRefs(parentAsItem));
      nsCOMPtr<nsIDocShell> parent(do_QueryInterface(parentAsItem));
      if (parent) {
        nsCOMPtr<nsIContentViewer> parentContentViewer;
        nsresult rv =
          parent->GetContentViewer(getter_AddRefs(parentContentViewer));
        if (NS_SUCCEEDED(rv) && parentContentViewer) {
          muCV = do_QueryInterface(parentContentViewer);
        }
      }
    }
    if (muCV) {
      muCV->SetPrevDocCharacterSet(aCharset);
    }
  }
}

nsISupports*
nsHtml5TreeOpExecutor::GetTarget()
{
  return mDocument;
}



void
nsHtml5TreeOpExecutor::UpdateChildCounts()
{
  
}

nsresult
nsHtml5TreeOpExecutor::FlushTags()
{
    return NS_OK;
}

void
nsHtml5TreeOpExecutor::PostEvaluateScript(nsIScriptElement *aElement)
{
  nsCOMPtr<nsIHTMLDocument> htmlDocument = do_QueryInterface(mDocument);
  NS_ASSERTION(htmlDocument, "Document didn't QI into HTML document.");
  htmlDocument->ScriptExecuted(aElement);
}

void
nsHtml5TreeOpExecutor::UpdateStyleSheet(nsIContent* aElement)
{
  
  
  EndDocUpdate();

  if (NS_UNLIKELY(!mParser)) {
    
    return;
  }

  nsCOMPtr<nsIStyleSheetLinkingElement> ssle(do_QueryInterface(aElement));
  NS_ASSERTION(ssle, "Node didn't QI to style.");

  ssle->SetEnableUpdates(PR_TRUE);

  PRBool willNotify;
  PRBool isAlternate;
  nsresult rv = ssle->UpdateStyleSheet(this, &willNotify, &isAlternate);
  if (NS_SUCCEEDED(rv) && willNotify && !isAlternate) {
    ++mPendingSheetCount;
    mScriptLoader->AddExecuteBlocker();
  }

  if (aElement->IsHTML() && aElement->Tag() == nsGkAtoms::link) {
    
    nsAutoString relVal;
    aElement->GetAttr(kNameSpaceID_None, nsGkAtoms::rel, relVal);
    if (!relVal.IsEmpty()) {
      
      nsAutoTArray<nsString, 4> linkTypes;
      nsStyleLinkElement::ParseLinkTypes(relVal, linkTypes);
      PRBool hasPrefetch = linkTypes.Contains(NS_LITERAL_STRING("prefetch"));
      if (hasPrefetch || linkTypes.Contains(NS_LITERAL_STRING("next"))) {
        nsAutoString hrefVal;
        aElement->GetAttr(kNameSpaceID_None, nsGkAtoms::href, hrefVal);
        if (!hrefVal.IsEmpty()) {
          PrefetchHref(hrefVal, aElement, hasPrefetch);
        }
      }
      if (linkTypes.Contains(NS_LITERAL_STRING("dns-prefetch"))) {
        nsAutoString hrefVal;
        aElement->GetAttr(kNameSpaceID_None, nsGkAtoms::href, hrefVal);
        if (!hrefVal.IsEmpty()) {
          PrefetchDNS(hrefVal);
        }
      }
    }
  }

  
  BeginDocUpdate();
}

class nsHtml5ExecutorReflusher : public nsRunnable
{
  private:
    nsRefPtr<nsHtml5TreeOpExecutor> mExecutor;
  public:
    nsHtml5ExecutorReflusher(nsHtml5TreeOpExecutor* aExecutor)
      : mExecutor(aExecutor)
    {}
    NS_IMETHODIMP Run()
    {
      mExecutor->Flush(PR_FALSE);
      return NS_OK;
    }
};

void
nsHtml5TreeOpExecutor::Flush(PRBool aForceWholeQueue)
{
  if (!mParser) {
    mOpQueue.Clear(); 
    return;
  }
  if (mFlushState != eNotFlushing) {
    return;
  }
  
  mFlushState = eInFlush;

  nsRefPtr<nsHtml5TreeOpExecutor> kungFuDeathGrip(this); 
  nsCOMPtr<nsIParser> parserKungFuDeathGrip(mParser);

  if (mReadingFromStage) {
    mStage.MoveOpsTo(mOpQueue);
  }
  
  nsIContent* scriptElement = nsnull;
  
  BeginDocUpdate();

  PRIntervalTime flushStart = 0;
  PRUint32 numberOfOpsToFlush = mOpQueue.Length();
  PRBool reflushNeeded = PR_FALSE;

#ifdef DEBUG_NS_HTML5_TREE_OP_EXECUTOR_FLUSH
  if (numberOfOpsToFlush > sOpQueueMaxLength) {
    sOpQueueMaxLength = numberOfOpsToFlush;
  }
  printf("QUEUE LENGTH: %d\n", numberOfOpsToFlush);
  printf("MAX QUEUE LENGTH: %d\n", sOpQueueMaxLength);
#endif

  if (aForceWholeQueue) {
    if (numberOfOpsToFlush > (PRUint32)sTreeOpQueueMinLength) {
      flushStart = PR_IntervalNow(); 
    }    
  } else {
    if (numberOfOpsToFlush > (PRUint32)sTreeOpQueueMinLength) {
      flushStart = PR_IntervalNow(); 
      if (numberOfOpsToFlush > (PRUint32)sTreeOpQueueLengthLimit) {
        numberOfOpsToFlush = (PRUint32)sTreeOpQueueLengthLimit;
        reflushNeeded = PR_TRUE;
      }
    }
  }

  mElementsSeenInThisAppendBatch.SetCapacity(numberOfOpsToFlush * 2);

  const nsHtml5TreeOperation* start = mOpQueue.Elements();
  const nsHtml5TreeOperation* end = start + numberOfOpsToFlush;
  for (nsHtml5TreeOperation* iter = (nsHtml5TreeOperation*)start; iter < end; ++iter) {
    if (NS_UNLIKELY(!mParser)) {
      
      break;
    }
    NS_ASSERTION(mFlushState == eInDocUpdate, "Tried to perform tree op outside update batch.");
    iter->Perform(this, &scriptElement);
  }

  if (NS_LIKELY(mParser)) {
    mOpQueue.RemoveElementsAt(0, numberOfOpsToFlush);  
  } else {
    mOpQueue.Clear(); 
  }
  
  if (flushStart) {
    PRUint32 delta = PR_IntervalToMilliseconds(PR_IntervalNow() - flushStart);
    sTreeOpQueueLengthLimit = delta ?
      (PRUint32)(((PRUint64)sTreeOpQueueMaxTime * (PRUint64)numberOfOpsToFlush)
                 / delta) :
      sTreeOpQueueMaxLength; 
    if (sTreeOpQueueLengthLimit < sTreeOpQueueMinLength) {
      
      
      sTreeOpQueueLengthLimit = sTreeOpQueueMinLength;
    }
    if (sTreeOpQueueLengthLimit > sTreeOpQueueMaxLength) {
      sTreeOpQueueLengthLimit = sTreeOpQueueMaxLength;
    }
#ifdef DEBUG_NS_HTML5_TREE_OP_EXECUTOR_FLUSH
    printf("FLUSH DURATION (millis): %d\n", delta);
    printf("QUEUE NEW MAX LENGTH: %d\n", sTreeOpQueueLengthLimit);      
#endif
  }

  EndDocUpdate();

  mFlushState = eNotFlushing;

  if (NS_UNLIKELY(!mParser)) {
    return;
  }

  if (scriptElement) {
    NS_ASSERTION(!reflushNeeded, "Got scriptElement when queue not fully flushed.");
    RunScript(scriptElement); 
  } else if (reflushNeeded) {
#ifdef DEBUG_NS_HTML5_TREE_OP_EXECUTOR_FLUSH
    printf("REFLUSH SCHEDULED.\n");
#endif
    nsCOMPtr<nsIRunnable> flusher = new nsHtml5ExecutorReflusher(this);  
    if (NS_FAILED(NS_DispatchToMainThread(flusher))) {
      NS_WARNING("failed to dispatch executor flush event");
    }
  }
}

nsresult
nsHtml5TreeOpExecutor::ProcessBASETag(nsIContent* aContent)
{
  NS_ASSERTION(aContent, "missing base-element");
  if (mHasProcessedBase) {
    return NS_OK;
  }
  mHasProcessedBase = PR_TRUE;
  nsresult rv = NS_OK;
  if (mDocument) {
    nsAutoString value;
    if (aContent->GetAttr(kNameSpaceID_None, nsHtml5Atoms::target, value)) {
      mDocument->SetBaseTarget(value);
    }
    if (aContent->GetAttr(kNameSpaceID_None, nsHtml5Atoms::href, value)) {
      nsCOMPtr<nsIURI> baseURI;
      rv = NS_NewURI(getter_AddRefs(baseURI), value);
      if (NS_SUCCEEDED(rv)) {
        rv = mDocument->SetBaseURI(baseURI); 
        if (NS_SUCCEEDED(rv)) {
          mDocumentBaseURI = mDocument->GetBaseURI();
        }
      }
    }
  }
  return rv;
}


PRBool
nsHtml5TreeOpExecutor::IsScriptEnabled()
{
  NS_ENSURE_TRUE(mDocument && mDocShell, PR_TRUE);
  nsCOMPtr<nsIScriptGlobalObject> globalObject = mDocument->GetScriptGlobalObject();
  
  
  if (!globalObject) {
    nsCOMPtr<nsIScriptGlobalObjectOwner> owner = do_GetInterface(mDocShell);
    NS_ENSURE_TRUE(owner, PR_TRUE);
    globalObject = owner->GetScriptGlobalObject();
    NS_ENSURE_TRUE(globalObject, PR_TRUE);
  }
  nsIScriptContext *scriptContext = globalObject->GetContext();
  NS_ENSURE_TRUE(scriptContext, PR_TRUE);
  JSContext* cx = (JSContext *) scriptContext->GetNativeContext();
  NS_ENSURE_TRUE(cx, PR_TRUE);
  PRBool enabled = PR_TRUE;
  nsContentUtils::GetSecurityManager()->
    CanExecuteScripts(cx, mDocument->NodePrincipal(), &enabled);
  return enabled;
}

void
nsHtml5TreeOpExecutor::DocumentMode(nsHtml5DocumentMode m)
{
  nsCompatibility mode = eCompatibility_NavQuirks;
  switch (m) {
    case STANDARDS_MODE:
      mode = eCompatibility_FullStandards;
      break;
    case ALMOST_STANDARDS_MODE:
      mode = eCompatibility_AlmostStandards;
      break;
    case QUIRKS_MODE:
      mode = eCompatibility_NavQuirks;
      break;
  }
  nsCOMPtr<nsIHTMLDocument> htmlDocument = do_QueryInterface(mDocument);
  NS_ASSERTION(htmlDocument, "Document didn't QI into HTML document.");
  htmlDocument->SetCompatibilityMode(mode);
}












void
nsHtml5TreeOpExecutor::RunScript(nsIContent* aScriptElement)
{
  NS_ASSERTION(aScriptElement, "No script to run");
  nsCOMPtr<nsIScriptElement> sele = do_QueryInterface(aScriptElement);
  
  if (!mParser) {
    NS_ASSERTION(sele->IsMalformed(), "Script wasn't marked as malformed.");
    
    
    
    return;
  }
  
  if (mFragmentMode) {
    
    
    sele->PreventExecution();
    return;
  }

  if (sele->GetScriptDeferred() || sele->GetScriptAsync()) {
    #ifdef DEBUG
    nsresult rv = 
    #endif
    aScriptElement->DoneAddingChildren(PR_TRUE); 
    NS_ASSERTION(rv != NS_ERROR_HTMLPARSER_BLOCK, 
                 "Defer or async script tried to block.");
    return;
  }
  
  NS_ASSERTION(mFlushState == eNotFlushing, "Tried to run script when flushing.");

  mReadingFromStage = PR_FALSE;
  
  sele->SetCreatorParser(mParser);

  
  nsCOMPtr<nsIHTMLDocument> htmlDocument = do_QueryInterface(mDocument);
  NS_ASSERTION(htmlDocument, "Document didn't QI into HTML document.");
  htmlDocument->ScriptLoading(sele);

  
  
  
  
  nsresult rv = aScriptElement->DoneAddingChildren(PR_TRUE);

  
  
  if (rv == NS_ERROR_HTMLPARSER_BLOCK) {
    mScriptElements.AppendObject(sele);
    mParser->BlockParser();
  } else {
    
    
    htmlDocument->ScriptExecuted(sele);
    
    ContinueInterruptedParsingAsync();
  }
}

nsresult
nsHtml5TreeOpExecutor::Init(nsIDocument* aDoc,
                            nsIURI* aURI,
                            nsISupports* aContainer,
                            nsIChannel* aChannel)
{
  nsresult rv = nsContentSink::Init(aDoc, aURI, aContainer, aChannel);
  NS_ENSURE_SUCCESS(rv, rv);
  mCanInterruptParser = PR_FALSE; 
                                  
  return rv;
}

void
nsHtml5TreeOpExecutor::Start()
{
  NS_PRECONDITION(!mStarted, "Tried to start when already started.");
  mStarted = PR_TRUE;
}

void
nsHtml5TreeOpExecutor::NeedsCharsetSwitchTo(const char* aEncoding)
{
  EndDocUpdate();

  if(NS_UNLIKELY(!mParser)) {
    
    return;
  }
  
  nsCOMPtr<nsIWebShellServices> wss = do_QueryInterface(mDocShell);
  if (!wss) {
    return;
  }

  
  if (NS_SUCCEEDED(wss->StopDocumentLoad())) {
    wss->ReloadDocument(aEncoding, kCharsetFromMetaTag);
  }
  
  

  if (!mParser) {
    
    return;
  }

  (static_cast<nsHtml5Parser*> (mParser.get()))->ContinueAfterFailedCharsetSwitch();

  BeginDocUpdate();
}

nsHtml5Tokenizer*
nsHtml5TreeOpExecutor::GetTokenizer()
{
  return (static_cast<nsHtml5Parser*> (mParser.get()))->GetTokenizer();
}

void
nsHtml5TreeOpExecutor::Reset() {
  mHasProcessedBase = PR_FALSE;
  mReadingFromStage = PR_FALSE;
  mOpQueue.Clear();
  mStarted = PR_FALSE;
  mFlushState = eNotFlushing;
  mFragmentMode = PR_FALSE;
}

void
nsHtml5TreeOpExecutor::MoveOpsFrom(nsTArray<nsHtml5TreeOperation>& aOpQueue)
{
  NS_PRECONDITION(mFlushState == eNotFlushing, "mOpQueue modified during tree op execution.");
  if (mOpQueue.IsEmpty()) {
    mOpQueue.SwapElements(aOpQueue);
    return;
  }
  mOpQueue.MoveElementsFrom(aOpQueue);
}

void
nsHtml5TreeOpExecutor::InitializeDocWriteParserState(nsAHtml5TreeBuilderState* aState, PRInt32 aLine)
{
  static_cast<nsHtml5Parser*> (mParser.get())->InitializeDocWriteParserState(aState, aLine);
}

#ifdef DEBUG_NS_HTML5_TREE_OP_EXECUTOR_FLUSH
PRUint32 nsHtml5TreeOpExecutor::sOpQueueMaxLength = 0;
PRUint32 nsHtml5TreeOpExecutor::sAppendBatchMaxSize = 0;
PRUint32 nsHtml5TreeOpExecutor::sAppendBatchSlotsExamined = 0;
PRUint32 nsHtml5TreeOpExecutor::sAppendBatchExaminations = 0;
#endif
