







































#include "nsHtml5TreeOpExecutor.h"
#include "nsScriptLoader.h"
#include "nsIMarkupDocumentViewer.h"
#include "nsIContentViewer.h"
#include "nsIDocShellTreeItem.h"
#include "nsIStyleSheetLinkingElement.h"
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

#define NS_HTML5_TREE_OP_EXECUTOR_MAX_QUEUE_TIME 3000UL // milliseconds
#define NS_HTML5_TREE_OP_EXECUTOR_DEFAULT_QUEUE_LENGTH 200
#define NS_HTML5_TREE_OP_EXECUTOR_MIN_QUEUE_LENGTH 100
#define NS_HTML5_TREE_OP_EXECUTOR_MAX_TIME_WITHOUT_FLUSH 5000 // milliseconds

NS_IMPL_CYCLE_COLLECTION_CLASS(nsHtml5TreeOpExecutor)

NS_INTERFACE_TABLE_HEAD_CYCLE_COLLECTION_INHERITED(nsHtml5TreeOpExecutor)
  NS_INTERFACE_TABLE_INHERITED1(nsHtml5TreeOpExecutor, 
                                nsIContentSink)
NS_INTERFACE_TABLE_TAIL_INHERITING(nsContentSink)

NS_IMPL_ADDREF_INHERITED(nsHtml5TreeOpExecutor, nsContentSink)

NS_IMPL_RELEASE_INHERITED(nsHtml5TreeOpExecutor, nsContentSink)

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(nsHtml5TreeOpExecutor, nsContentSink)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mFlushTimer)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mScriptElement)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMARRAY(mOwnedElements)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMARRAY(mOwnedNonElements)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(nsHtml5TreeOpExecutor, nsContentSink)
  if (tmp->mFlushTimer) {
    tmp->mFlushTimer->Cancel();
  }
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mFlushTimer)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mScriptElement)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMARRAY(mOwnedElements)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMARRAY(mOwnedNonElements)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

nsHtml5TreeOpExecutor::nsHtml5TreeOpExecutor()
  : mSuppressEOF(PR_FALSE)
  , mHasProcessedBase(PR_FALSE)
  , mNeedsFlush(PR_FALSE)
  , mFlushTimer(do_CreateInstance("@mozilla.org/timer;1"))
  , mNeedsCharsetSwitch(PR_FALSE)
{

}

nsHtml5TreeOpExecutor::~nsHtml5TreeOpExecutor()
{
  NS_ASSERTION(mOpQueue.IsEmpty(), "Somehow there's stuff in the op queue.");
  if (mFlushTimer) {
    mFlushTimer->Cancel(); 
  }
}

static void
TimerCallbackFunc(nsITimer* aTimer, void* aClosure)
{
  (static_cast<nsHtml5TreeOpExecutor*> (aClosure))->DeferredTimerFlush();
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
  NS_ASSERTION(mLifeCycle == STREAM_ENDING, "Bad life cycle.");
  mLifeCycle = TERMINATED;
  if (!mSuppressEOF) {
    GetTokenizer()->eof();
    Flush();
  }
  GetTokenizer()->end();
  
  DidBuildModelImpl(aTerminated);
  mDocument->ScriptLoader()->RemoveObserver(this);
  nsContentSink::StartLayout(PR_FALSE);
  ScrollToRef();
  mDocument->RemoveObserver(this);
  mDocument->EndLoad();
  static_cast<nsHtml5Parser*> (mParser.get())->DropStreamParser();
  static_cast<nsHtml5Parser*> (mParser.get())->CancelParsingEvents();
  DropParserAndPerfHint();
#ifdef GATHER_DOCWRITE_STATISTICS
  printf("UNSAFE SCRIPTS: %d\n", sUnsafeDocWrites);
  printf("TOKENIZER-SAFE SCRIPTS: %d\n", sTokenSafeDocWrites);
  printf("TREEBUILDER-SAFE SCRIPTS: %d\n", sTreeSafeDocWrites);
#endif
#ifdef DEBUG_hsivonen
  printf("MAX INSERTION BATCH LEN: %d\n", sInsertionBatchMaxLength);
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

NS_IMETHODIMP
nsHtml5TreeOpExecutor::SetDocumentCharset(nsACString& aCharset)
{
  if (mDocShell) {
    
    
    
    
    nsCOMPtr<nsIMarkupDocumentViewer> muCV;
    nsCOMPtr<nsIContentViewer> cv;
    mDocShell->GetContentViewer(getter_AddRefs(cv));
    if (cv) {
      muCV = do_QueryInterface(cv);
    } else {
      
      
      
      nsCOMPtr<nsIDocShellTreeItem> docShellAsItem =
        do_QueryInterface(mDocShell);
      NS_ENSURE_TRUE(docShellAsItem, NS_ERROR_FAILURE);
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
  if (mDocument) {
    mDocument->SetDocumentCharacterSet(aCharset);
  }
  return NS_OK;
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
  nsCOMPtr<nsIStyleSheetLinkingElement> ssle(do_QueryInterface(aElement));
  if (ssle) {
    ssle->SetEnableUpdates(PR_TRUE);
    PRBool willNotify;
    PRBool isAlternate;
    nsresult rv = ssle->UpdateStyleSheet(this, &willNotify, &isAlternate);
    if (NS_SUCCEEDED(rv) && willNotify && !isAlternate) {
      ++mPendingSheetCount;
      mScriptLoader->AddExecuteBlocker();
    }
  }
}

void
nsHtml5TreeOpExecutor::Flush()
{
  mNeedsFlush = PR_FALSE;
  FillQueue();
  
  MOZ_AUTO_DOC_UPDATE(GetDocument(), UPDATE_CONTENT_MODEL, PR_TRUE);
  PRIntervalTime flushStart = 0;
  PRUint32 opQueueLength = mOpQueue.Length();
  if (opQueueLength > NS_HTML5_TREE_OP_EXECUTOR_MIN_QUEUE_LENGTH) { 
    flushStart = PR_IntervalNow();
  }
  mElementsSeenInThisAppendBatch.SetCapacity(opQueueLength * 2);
  
  const nsHtml5TreeOperation* start = mOpQueue.Elements();
  const nsHtml5TreeOperation* end = start + opQueueLength;
  for (nsHtml5TreeOperation* iter = (nsHtml5TreeOperation*)start; iter < end; ++iter) {
    iter->Perform(this);
  }
  FlushPendingAppendNotifications();
#ifdef DEBUG_hsivonen
  if (mOpQueue.Length() > sInsertionBatchMaxLength) {
    sInsertionBatchMaxLength = opQueueLength;
  }
#endif
  mOpQueue.Clear();
  if (flushStart) {
    PRUint32 delta = PR_IntervalToMilliseconds(PR_IntervalNow() - flushStart);
    sTreeOpQueueMaxLength = delta ?
      (PRUint32)((NS_HTML5_TREE_OP_EXECUTOR_MAX_QUEUE_TIME * (PRUint64)opQueueLength) / delta) :
      0;
    if (sTreeOpQueueMaxLength < NS_HTML5_TREE_OP_EXECUTOR_MIN_QUEUE_LENGTH) {
      sTreeOpQueueMaxLength = NS_HTML5_TREE_OP_EXECUTOR_MIN_QUEUE_LENGTH;
    }
#ifdef DEBUG_hsivonen
    printf("QUEUE MAX LENGTH: %d\n", sTreeOpQueueMaxLength);
#endif
  }
  mFlushTimer->InitWithFuncCallback(TimerCallbackFunc, static_cast<void*> (this), NS_HTML5_TREE_OP_EXECUTOR_MAX_TIME_WITHOUT_FLUSH, nsITimer::TYPE_ONE_SHOT);
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

nsresult
nsHtml5TreeOpExecutor::MaybePerformCharsetSwitch()
{
  if (!mNeedsCharsetSwitch) {
    return NS_ERROR_HTMLPARSER_CONTINUE;
  }
  
  nsresult rv = NS_OK;
  nsCOMPtr<nsIWebShellServices> wss = do_QueryInterface(mDocShell);
  if (!wss) {
    return NS_ERROR_HTMLPARSER_CONTINUE;
  }
#ifndef DONT_INFORM_WEBSHELL
  
  if (NS_FAILED(rv = wss->SetRendering(PR_FALSE))) {
    
  } else if (NS_FAILED(rv = wss->StopDocumentLoad())) {
    rv = wss->SetRendering(PR_TRUE); 
  } else if (NS_FAILED(rv = wss->ReloadDocument(mPendingCharset.get(), kCharsetFromMetaTag))) {
    rv = wss->SetRendering(PR_TRUE); 
  } else {
    rv = NS_ERROR_HTMLPARSER_STOPPARSING; 
  }
#endif
  
  if (rv != NS_ERROR_HTMLPARSER_STOPPARSING)
    mNeedsCharsetSwitch = PR_FALSE;
    rv = NS_ERROR_HTMLPARSER_CONTINUE;
  return rv;
}







void
nsHtml5TreeOpExecutor::ExecuteScript()
{
  NS_PRECONDITION(mScriptElement, "Trying to run a script without having one!");
  Flush();
#ifdef GATHER_DOCWRITE_STATISTICS
  if (!mSnapshot) {
    mSnapshot = mTreeBuilder->newSnapshot();
  }
#endif
  nsCOMPtr<nsIScriptElement> sele = do_QueryInterface(mScriptElement);
   
  nsCOMPtr<nsIHTMLDocument> htmlDocument = do_QueryInterface(mDocument);
  NS_ASSERTION(htmlDocument, "Document didn't QI into HTML document.");
  htmlDocument->ScriptLoading(sele);
   
  
  
  
  nsresult rv = mScriptElement->DoneAddingChildren(PR_TRUE);
  
  
  if (rv == NS_ERROR_HTMLPARSER_BLOCK) {
    mScriptElements.AppendObject(sele);
    mParser->BlockParser();
  } else {
    
    
    htmlDocument->ScriptExecuted(sele);
  }
  mScriptElement = nsnull;
}

nsresult
nsHtml5TreeOpExecutor::Init(nsIDocument* aDoc,
                            nsIURI* aURI,
                            nsISupports* aContainer,
                            nsIChannel* aChannel)
{
  nsresult rv = nsContentSink::Init(aDoc, aURI, aContainer, aChannel);
  NS_ENSURE_SUCCESS(rv, rv);
  aDoc->AddObserver(this);
  return rv;
}

void
nsHtml5TreeOpExecutor::Start()
{
  mNeedsFlush = PR_FALSE;
  mNeedsCharsetSwitch = PR_FALSE;
  mPendingCharset.Truncate();
  mScriptElement = nsnull;
}

void
nsHtml5TreeOpExecutor::End()
{
  mFlushTimer->Cancel();
}

void
nsHtml5TreeOpExecutor::NeedsCharsetSwitchTo(const nsACString& aEncoding)
{
  mNeedsCharsetSwitch = PR_TRUE;
  mPendingCharset.Assign(aEncoding);
}

nsHtml5Tokenizer*
nsHtml5TreeOpExecutor::GetTokenizer()
{
  return (static_cast<nsHtml5Parser*> (mParser.get()))->GetTokenizer();
}

void
nsHtml5TreeOpExecutor::MaybeSuspend() {
  if (!mNeedsFlush) {
    mNeedsFlush = !!(mTreeBuilder->GetOpQueueLength() >= sTreeOpQueueMaxLength);
  }
  if (DidProcessATokenImpl() == NS_ERROR_HTMLPARSER_INTERRUPTED || mNeedsFlush) {
    
    
    static_cast<nsHtml5Parser*>(mParser.get())->Suspend();
    mTreeBuilder->ReqSuspension();
  }
}

void
nsHtml5TreeOpExecutor::MaybeExecuteScript() {
  if (!mTreeBuilder->HasScript()) {
    return;
  }
  Flush(); 
  NS_ASSERTION(mScriptElement, "No script to run");
  ExecuteScript();
  if (mStreamParser) {
    mStreamParser->Suspend();
  }
}

void
nsHtml5TreeOpExecutor::DeferredTimerFlush() {
  if (mTreeBuilder->GetOpQueueLength() > 0) {
    mNeedsFlush = PR_TRUE;
  }
}

void
nsHtml5TreeOpExecutor::FillQueue() {
  mTreeBuilder->SwapQueue(mOpQueue);
}

void
nsHtml5TreeOpExecutor::Reset() {
  mSuppressEOF = PR_FALSE;
  mHasProcessedBase = PR_FALSE;
  mNeedsFlush = PR_FALSE;
  mOpQueue.Clear();
  mPendingCharset.Truncate();
  mNeedsCharsetSwitch = PR_FALSE;
  mLifeCycle = NOT_STARTED;
  mScriptElement = nsnull;
}

PRUint32 nsHtml5TreeOpExecutor::sTreeOpQueueMaxLength = NS_HTML5_TREE_OP_EXECUTOR_DEFAULT_QUEUE_LENGTH;
#ifdef DEBUG_hsivonen
PRUint32 nsHtml5TreeOpExecutor::sInsertionBatchMaxLength = 0;
PRUint32 nsHtml5TreeOpExecutor::sAppendBatchMaxSize = 0;
PRUint32 nsHtml5TreeOpExecutor::sAppendBatchSlotsExamined = 0;
PRUint32 nsHtml5TreeOpExecutor::sAppendBatchExaminations = 0;
#endif
