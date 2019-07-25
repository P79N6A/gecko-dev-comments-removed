







































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
#include "mozilla/css/Loader.h"
#include "mozilla/Util.h" 

using namespace mozilla;

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
      mExecutor->RunFlushLoop();
      return NS_OK;
    }
};

nsHtml5TreeOpExecutor::nsHtml5TreeOpExecutor()
{
  mPreloadedURLs.Init(23); 
  
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
nsHtml5TreeOpExecutor::DidBuildModel(bool aTerminated)
{
  NS_PRECONDITION(mStarted, "Bad life cycle.");

  if (!aTerminated) {
    
    
    
    EndDocUpdate();
    
    
    
    if (!mParser) {
      return NS_OK;
    }
  }
  
  GetParser()->DropStreamParser();

  
  DidBuildModelImpl(aTerminated);

  if (!mLayoutStarted) {
    
    

    
    
    
    
    bool destroying = true;
    if (mDocShell) {
      mDocShell->IsBeingDestroyed(&destroying);
    }

    if (!destroying) {
      nsContentSink::StartLayout(false);
    }
  }

  ScrollToRef();
  mDocument->ScriptLoader()->RemoveObserver(this);
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
  NS_NOTREACHED("Don't call. For interface compat only.");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsHtml5TreeOpExecutor::WillResume()
{
  NS_NOTREACHED("Don't call. For interface compat only.");
  return NS_ERROR_NOT_IMPLEMENTED;
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
  if (aType >= Flush_InterruptibleLayout) {
    
    nsContentSink::StartLayout(true);
  }
}

void
nsHtml5TreeOpExecutor::SetDocumentCharsetAndSource(nsACString& aCharset, PRInt32 aCharsetSource)
{
  if (mDocument) {
    mDocument->SetDocumentCharacterSetSource(aCharsetSource);
    mDocument->SetDocumentCharacterSet(aCharset);
  }
  if (mDocShell) {
    
    
    
    
    nsCOMPtr<nsIMarkupDocumentViewer> mucv;
    nsCOMPtr<nsIContentViewer> cv;
    mDocShell->GetContentViewer(getter_AddRefs(cv));
    if (cv) {
      mucv = do_QueryInterface(cv);
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
          mucv = do_QueryInterface(parentContentViewer);
        }
      }
    }
    if (mucv) {
      mucv->SetPrevDocCharacterSet(aCharset);
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

void
nsHtml5TreeOpExecutor::MarkAsBroken()
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(!mFragmentMode, "Fragment parsers can't be broken!");
  mBroken = true;
  if (mStreamParser) {
    mStreamParser->Terminate();
  }
  
  
  
  if (mParser) { 
    nsCOMPtr<nsIRunnable> terminator =
      NS_NewRunnableMethod(GetParser(), &nsHtml5Parser::Terminate);
    if (NS_FAILED(NS_DispatchToMainThread(terminator))) {
      NS_WARNING("failed to dispatch executor flush event");
    }
  }
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
nsHtml5TreeOpExecutor::ContinueInterruptedParsingAsync()
{
  nsCOMPtr<nsIRunnable> flusher = new nsHtml5ExecutorReflusher(this);  
  if (NS_FAILED(NS_DispatchToMainThread(flusher))) {
    NS_WARNING("failed to dispatch executor flush event");
  }          
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

  ssle->SetEnableUpdates(true);

  bool willNotify;
  bool isAlternate;
  nsresult rv = ssle->UpdateStyleSheet(mFragmentMode ? nsnull : this,
                                       &willNotify,
                                       &isAlternate);
  if (NS_SUCCEEDED(rv) && willNotify && !isAlternate && !mFragmentMode) {
    ++mPendingSheetCount;
    mScriptLoader->AddExecuteBlocker();
  }

  if (aElement->IsHTML(nsGkAtoms::link)) {
    
    nsAutoString relVal;
    aElement->GetAttr(kNameSpaceID_None, nsGkAtoms::rel, relVal);
    if (!relVal.IsEmpty()) {
      
      nsAutoTArray<nsString, 4> linkTypes;
      nsStyleLinkElement::ParseLinkTypes(relVal, linkTypes);
      bool hasPrefetch = linkTypes.Contains(NS_LITERAL_STRING("prefetch"));
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

void
nsHtml5TreeOpExecutor::FlushSpeculativeLoads()
{
  nsTArray<nsHtml5SpeculativeLoad> speculativeLoadQueue;
  mStage.MoveSpeculativeLoadsTo(speculativeLoadQueue);
  const nsHtml5SpeculativeLoad* start = speculativeLoadQueue.Elements();
  const nsHtml5SpeculativeLoad* end = start + speculativeLoadQueue.Length();
  for (nsHtml5SpeculativeLoad* iter = const_cast<nsHtml5SpeculativeLoad*>(start);
       iter < end;
       ++iter) {
    if (NS_UNLIKELY(!mParser)) {
      
      return;
    }
    iter->Perform(this);
  }
}

class nsHtml5FlushLoopGuard
{
  private:
    nsRefPtr<nsHtml5TreeOpExecutor> mExecutor;
    #ifdef DEBUG_NS_HTML5_TREE_OP_EXECUTOR_FLUSH
    PRUint32 mStartTime;
    #endif
  public:
    nsHtml5FlushLoopGuard(nsHtml5TreeOpExecutor* aExecutor)
      : mExecutor(aExecutor)
    #ifdef DEBUG_NS_HTML5_TREE_OP_EXECUTOR_FLUSH
      , mStartTime(PR_IntervalToMilliseconds(PR_IntervalNow()))
    #endif
    {
      mExecutor->mRunFlushLoopOnStack = true;
    }
    ~nsHtml5FlushLoopGuard()
    {
      #ifdef DEBUG_NS_HTML5_TREE_OP_EXECUTOR_FLUSH
        PRUint32 timeOffTheEventLoop = 
          PR_IntervalToMilliseconds(PR_IntervalNow()) - mStartTime;
        if (timeOffTheEventLoop > 
            nsHtml5TreeOpExecutor::sLongestTimeOffTheEventLoop) {
          nsHtml5TreeOpExecutor::sLongestTimeOffTheEventLoop = 
            timeOffTheEventLoop;
        }
        printf("Longest time off the event loop: %d\n", 
          nsHtml5TreeOpExecutor::sLongestTimeOffTheEventLoop);
      #endif

      mExecutor->mRunFlushLoopOnStack = false;
    }
};




void
nsHtml5TreeOpExecutor::RunFlushLoop()
{
  if (mRunFlushLoopOnStack) {
    
    return;
  }
  
  nsHtml5FlushLoopGuard guard(this); 
  
  nsCOMPtr<nsIParser> parserKungFuDeathGrip(mParser);

  
  (void) nsContentSink::WillParseImpl();

  for (;;) {
    if (!mParser) {
      
      mOpQueue.Clear(); 
      return;
    }

    if (IsBroken()) {
      return;
    }

    if (!mParser->IsParserEnabled()) {
      
      return;
    }
  
    if (mFlushState != eNotFlushing) {
      
      return;
    }
    
    
    
    
    if (IsScriptExecuting()) {
      return;
    }

    if (mReadingFromStage) {
      nsTArray<nsHtml5SpeculativeLoad> speculativeLoadQueue;
      mStage.MoveOpsAndSpeculativeLoadsTo(mOpQueue, speculativeLoadQueue);
      
      
      const nsHtml5SpeculativeLoad* start = speculativeLoadQueue.Elements();
      const nsHtml5SpeculativeLoad* end = start + speculativeLoadQueue.Length();
      for (nsHtml5SpeculativeLoad* iter = (nsHtml5SpeculativeLoad*)start;
           iter < end;
           ++iter) {
        iter->Perform(this);
        if (NS_UNLIKELY(!mParser)) {
          
          mOpQueue.Clear(); 
          return;
        }
      }
    } else {
      FlushSpeculativeLoads(); 
                               
                               
      if (NS_UNLIKELY(!mParser)) {
        
        mOpQueue.Clear(); 
        return;
      }
      
      
      nsRefPtr<nsHtml5StreamParser> streamKungFuDeathGrip = 
        GetParser()->GetStreamParser();
      
      
      GetParser()->ParseUntilBlocked();
    }

    if (mOpQueue.IsEmpty()) {
      
      
      return;
    }

    mFlushState = eInFlush;

    nsIContent* scriptElement = nsnull;
    
    BeginDocUpdate();

    PRUint32 numberOfOpsToFlush = mOpQueue.Length();

    mElementsSeenInThisAppendBatch.SetCapacity(numberOfOpsToFlush * 2);

    const nsHtml5TreeOperation* first = mOpQueue.Elements();
    const nsHtml5TreeOperation* last = first + numberOfOpsToFlush - 1;
    for (nsHtml5TreeOperation* iter = const_cast<nsHtml5TreeOperation*>(first);;) {
      if (NS_UNLIKELY(!mParser)) {
        
        break;
      }
      NS_ASSERTION(mFlushState == eInDocUpdate, 
        "Tried to perform tree op outside update batch.");
      iter->Perform(this, &scriptElement);

      
      if (NS_UNLIKELY(iter == last)) {
        break;
      } else if (NS_UNLIKELY(nsContentSink::DidProcessATokenImpl() == 
                             NS_ERROR_HTMLPARSER_INTERRUPTED)) {
        mOpQueue.RemoveElementsAt(0, (iter - first) + 1);
        
        EndDocUpdate();

        mFlushState = eNotFlushing;

        #ifdef DEBUG_NS_HTML5_TREE_OP_EXECUTOR_FLUSH
          printf("REFLUSH SCHEDULED (executing ops): %d\n", 
            ++sTimesFlushLoopInterrupted);
        #endif
        nsHtml5TreeOpExecutor::ContinueInterruptedParsingAsync();
        return;
      }
      ++iter;
    }
    
    mOpQueue.Clear();
    
    EndDocUpdate();

    mFlushState = eNotFlushing;

    if (NS_UNLIKELY(!mParser)) {
      
      return;
    }

    if (scriptElement) {
      
      RunScript(scriptElement);
      
      
      if (nsContentSink::DidProcessATokenImpl() == 
          NS_ERROR_HTMLPARSER_INTERRUPTED) {
        #ifdef DEBUG_NS_HTML5_TREE_OP_EXECUTOR_FLUSH
          printf("REFLUSH SCHEDULED (after script): %d\n", 
            ++sTimesFlushLoopInterrupted);
        #endif
        nsHtml5TreeOpExecutor::ContinueInterruptedParsingAsync();
        return;      
      }
    }
  }
}

void
nsHtml5TreeOpExecutor::FlushDocumentWrite()
{
  FlushSpeculativeLoads(); 
                

  if (NS_UNLIKELY(!mParser)) {
    
    mOpQueue.Clear(); 
    return;
  }
  
  if (mFlushState != eNotFlushing) {
    
    return;
  }

  mFlushState = eInFlush;

  
  nsRefPtr<nsHtml5TreeOpExecutor> kungFuDeathGrip(this);
  nsCOMPtr<nsIParser> parserKungFuDeathGrip(mParser);

  NS_ASSERTION(!mReadingFromStage,
    "Got doc write flush when reading from stage");

#ifdef DEBUG
  mStage.AssertEmpty();
#endif
  
  nsIContent* scriptElement = nsnull;
  
  BeginDocUpdate();

  PRUint32 numberOfOpsToFlush = mOpQueue.Length();

  mElementsSeenInThisAppendBatch.SetCapacity(numberOfOpsToFlush * 2);

  const nsHtml5TreeOperation* start = mOpQueue.Elements();
  const nsHtml5TreeOperation* end = start + numberOfOpsToFlush;
  for (nsHtml5TreeOperation* iter = const_cast<nsHtml5TreeOperation*>(start);
       iter < end;
       ++iter) {
    if (NS_UNLIKELY(!mParser)) {
      
      break;
    }
    NS_ASSERTION(mFlushState == eInDocUpdate, 
      "Tried to perform tree op outside update batch.");
    iter->Perform(this, &scriptElement);
  }

  mOpQueue.Clear();
  
  EndDocUpdate();

  mFlushState = eNotFlushing;

  if (NS_UNLIKELY(!mParser)) {
    
    return;
  }

  if (scriptElement) {
    
    RunScript(scriptElement);
  }
}


bool
nsHtml5TreeOpExecutor::IsScriptEnabled()
{
  if (!mDocument || !mDocShell)
    return true;
  nsCOMPtr<nsIScriptGlobalObject> globalObject = mDocument->GetScriptGlobalObject();
  
  
  if (!globalObject) {
    nsCOMPtr<nsIScriptGlobalObjectOwner> owner = do_GetInterface(mDocShell);
    NS_ENSURE_TRUE(owner, true);
    globalObject = owner->GetScriptGlobalObject();
    NS_ENSURE_TRUE(globalObject, true);
  }
  nsIScriptContext *scriptContext = globalObject->GetContext();
  NS_ENSURE_TRUE(scriptContext, true);
  JSContext* cx = scriptContext->GetNativeContext();
  NS_ENSURE_TRUE(cx, true);
  bool enabled = true;
  nsContentUtils::GetSecurityManager()->
    CanExecuteScripts(cx, mDocument->NodePrincipal(), &enabled);
  return enabled;
}

void
nsHtml5TreeOpExecutor::SetDocumentMode(nsHtml5DocumentMode m)
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
nsHtml5TreeOpExecutor::StartLayout() {
  if (mLayoutStarted || !mDocument) {
    return;
  }

  EndDocUpdate();

  if (NS_UNLIKELY(!mParser)) {
    
    return;
  }

  nsContentSink::StartLayout(false);

  BeginDocUpdate();
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
    if (mPreventScriptExecution) {
      sele->PreventExecution();
    }
    return;
  }

  if (sele->GetScriptDeferred() || sele->GetScriptAsync()) {
    #ifdef DEBUG
    nsresult rv = 
    #endif
    aScriptElement->DoneAddingChildren(true); 
    NS_ASSERTION(rv != NS_ERROR_HTMLPARSER_BLOCK, 
                 "Defer or async script tried to block.");
    return;
  }
  
  NS_ASSERTION(mFlushState == eNotFlushing, "Tried to run script when flushing.");

  mReadingFromStage = false;
  
  sele->SetCreatorParser(mParser);

  
  nsCOMPtr<nsIHTMLDocument> htmlDocument = do_QueryInterface(mDocument);
  NS_ASSERTION(htmlDocument, "Document didn't QI into HTML document.");
  htmlDocument->ScriptLoading(sele);

  
  
  
  
  nsresult rv = aScriptElement->DoneAddingChildren(true);

  
  
  if (rv == NS_ERROR_HTMLPARSER_BLOCK) {
    mScriptElements.AppendObject(sele);
    if (mParser) {
      mParser->BlockParser();
    }
  } else {
    
    
    htmlDocument->ScriptExecuted(sele);
    
    

    
    
    nsHtml5TreeOpExecutor::ContinueInterruptedParsingAsync();
  }
}

nsresult
nsHtml5TreeOpExecutor::Init(nsIDocument* aDoc,
                            nsIURI* aURI,
                            nsISupports* aContainer,
                            nsIChannel* aChannel)
{
  return nsContentSink::Init(aDoc, aURI, aContainer, aChannel);
}

void
nsHtml5TreeOpExecutor::Start()
{
  NS_PRECONDITION(!mStarted, "Tried to start when already started.");
  mStarted = true;
}

void
nsHtml5TreeOpExecutor::NeedsCharsetSwitchTo(const char* aEncoding,
                                            PRInt32 aSource)
{
  EndDocUpdate();

  if (NS_UNLIKELY(!mParser)) {
    
    return;
  }
  
  nsCOMPtr<nsIWebShellServices> wss = do_QueryInterface(mDocShell);
  if (!wss) {
    return;
  }

  
  if (NS_SUCCEEDED(wss->StopDocumentLoad())) {
    wss->ReloadDocument(aEncoding, aSource);
  }
  
  

  if (!mParser) {
    
    return;
  }

  GetParser()->ContinueAfterFailedCharsetSwitch();

  BeginDocUpdate();
}

nsHtml5Parser*
nsHtml5TreeOpExecutor::GetParser()
{
  return static_cast<nsHtml5Parser*>(mParser.get());
}

nsHtml5Tokenizer*
nsHtml5TreeOpExecutor::GetTokenizer()
{
  return GetParser()->GetTokenizer();
}

void
nsHtml5TreeOpExecutor::Reset()
{
  DropHeldElements();
  mReadingFromStage = false;
  mOpQueue.Clear();
  mStarted = false;
  mFlushState = eNotFlushing;
  mRunFlushLoopOnStack = false;
  NS_ASSERTION(!mBroken, "Fragment parser got broken.");
}

void
nsHtml5TreeOpExecutor::DropHeldElements()
{
  mScriptLoader = nsnull;
  mDocument = nsnull;
  mNodeInfoManager = nsnull;
  mCSSLoader = nsnull;
  mDocumentURI = nsnull;
  mDocShell = nsnull;
  mOwnedElements.Clear();
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
  GetParser()->InitializeDocWriteParserState(aState, aLine);
}

nsIURI*
nsHtml5TreeOpExecutor::GetViewSourceBaseURI()
{
  if (!mViewSourceBaseURI) {
    nsCOMPtr<nsIURI> orig = mDocument->GetOriginalURI();
    bool isViewSource;
    orig->SchemeIs("view-source", &isViewSource);
    if (isViewSource) {
      nsCOMPtr<nsINestedURI> nested = do_QueryInterface(orig);
      NS_ASSERTION(nested, "URI with scheme view-source didn't QI to nested!");
      nested->GetInnerURI(getter_AddRefs(mViewSourceBaseURI));
    } else {
      mViewSourceBaseURI = orig;
    }
  }
  return mViewSourceBaseURI;
}



already_AddRefed<nsIURI>
nsHtml5TreeOpExecutor::ConvertIfNotPreloadedYet(const nsAString& aURL)
{
  if (aURL.IsEmpty()) {
    return nsnull;
  }
  
  nsIURI* documentURI = mDocument->GetDocumentURI();
  
  nsIURI* documentBaseURI = mDocument->GetDocBaseURI();

  
  
  
  nsIURI* base = (documentURI == documentBaseURI) ?
                  (mSpeculationBaseURI ?
                   mSpeculationBaseURI.get() : documentURI)
                 : documentBaseURI;
  const nsCString& charset = mDocument->GetDocumentCharacterSet();
  nsCOMPtr<nsIURI> uri;
  nsresult rv = NS_NewURI(getter_AddRefs(uri), aURL, charset.get(), base);
  if (NS_FAILED(rv)) {
    NS_WARNING("Failed to create a URI");
    return nsnull;
  }
  nsCAutoString spec;
  uri->GetSpec(spec);
  if (mPreloadedURLs.Contains(spec)) {
    return nsnull;
  }
  mPreloadedURLs.Put(spec);
  return uri.forget();
}

void
nsHtml5TreeOpExecutor::PreloadScript(const nsAString& aURL,
                                     const nsAString& aCharset,
                                     const nsAString& aType)
{
  nsCOMPtr<nsIURI> uri = ConvertIfNotPreloadedYet(aURL);
  if (!uri) {
    return;
  }
  mDocument->ScriptLoader()->PreloadURI(uri, aCharset, aType);
}

void
nsHtml5TreeOpExecutor::PreloadStyle(const nsAString& aURL,
                                    const nsAString& aCharset)
{
  nsCOMPtr<nsIURI> uri = ConvertIfNotPreloadedYet(aURL);
  if (!uri) {
    return;
  }
  mDocument->PreloadStyle(uri, aCharset);
}

void
nsHtml5TreeOpExecutor::PreloadImage(const nsAString& aURL,
                                    const nsAString& aCrossOrigin)
{
  nsCOMPtr<nsIURI> uri = ConvertIfNotPreloadedYet(aURL);
  if (!uri) {
    return;
  }
  mDocument->MaybePreLoadImage(uri, aCrossOrigin);
}

void
nsHtml5TreeOpExecutor::SetSpeculationBase(const nsAString& aURL)
{
  if (mSpeculationBaseURI) {
    
    return;
  }
  const nsCString& charset = mDocument->GetDocumentCharacterSet();
  DebugOnly<nsresult> rv = NS_NewURI(getter_AddRefs(mSpeculationBaseURI), aURL,
                                     charset.get(), mDocument->GetDocumentURI());
  NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Failed to create a URI");
}

#ifdef DEBUG_NS_HTML5_TREE_OP_EXECUTOR_FLUSH
PRUint32 nsHtml5TreeOpExecutor::sAppendBatchMaxSize = 0;
PRUint32 nsHtml5TreeOpExecutor::sAppendBatchSlotsExamined = 0;
PRUint32 nsHtml5TreeOpExecutor::sAppendBatchExaminations = 0;
PRUint32 nsHtml5TreeOpExecutor::sLongestTimeOffTheEventLoop = 0;
PRUint32 nsHtml5TreeOpExecutor::sTimesFlushLoopInterrupted = 0;
#endif
