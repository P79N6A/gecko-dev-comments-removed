





#include "mozilla/DebugOnly.h"
#include "mozilla/Likely.h"

#include "nsError.h"
#include "nsHtml5TreeOpExecutor.h"
#include "nsScriptLoader.h"
#include "nsIContentViewer.h"
#include "nsIDocShellTreeItem.h"
#include "nsIDocShell.h"
#include "nsIScriptGlobalObject.h"
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
#include "GeckoProfiler.h"
#include "nsIScriptError.h"
#include "nsIScriptContext.h"
#include "mozilla/Preferences.h"
#include "nsIHTMLDocument.h"

using namespace mozilla;

NS_INTERFACE_TABLE_HEAD_CYCLE_COLLECTION_INHERITED(nsHtml5TreeOpExecutor)
  NS_INTERFACE_TABLE_INHERITED(nsHtml5TreeOpExecutor,
                               nsIContentSink)
NS_INTERFACE_TABLE_TAIL_INHERITING(nsHtml5DocumentBuilder)

NS_IMPL_ADDREF_INHERITED(nsHtml5TreeOpExecutor, nsContentSink)

NS_IMPL_RELEASE_INHERITED(nsHtml5TreeOpExecutor, nsContentSink)

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

static mozilla::LinkedList<nsHtml5TreeOpExecutor>* gBackgroundFlushList = nullptr;
static nsITimer* gFlushTimer = nullptr;

nsHtml5TreeOpExecutor::nsHtml5TreeOpExecutor()
  : nsHtml5DocumentBuilder(false)
  , mPreloadedURLs(23)  
{
  
}

nsHtml5TreeOpExecutor::~nsHtml5TreeOpExecutor()
{
  if (gBackgroundFlushList && isInList()) {
    mOpQueue.Clear();
    removeFrom(*gBackgroundFlushList);
    if (gBackgroundFlushList->isEmpty()) {
      delete gBackgroundFlushList;
      gBackgroundFlushList = nullptr;
      if (gFlushTimer) {
        gFlushTimer->Cancel();
        NS_RELEASE(gFlushTimer);
      }
    }
  }
  NS_ASSERTION(mOpQueue.IsEmpty(), "Somehow there's stuff in the op queue.");
}


NS_IMETHODIMP
nsHtml5TreeOpExecutor::WillParse()
{
  NS_NOTREACHED("No one should call this");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsHtml5TreeOpExecutor::WillBuildModel(nsDTDMode aDTDMode)
{
  mDocument->AddObserver(this);
  WillBuildModelImpl();
  GetDocument()->BeginLoad();
  if (mDocShell && !GetDocument()->GetWindow() &&
      !IsExternalViewSource()) {
    
    return MarkAsBroken(NS_ERROR_DOM_INVALID_STATE_ERR);
  }
  return NS_OK;
}



NS_IMETHODIMP
nsHtml5TreeOpExecutor::DidBuildModel(bool aTerminated)
{
  if (!aTerminated) {
    
    
    
    EndDocUpdate();
    
    
    
    if (!mParser) {
      return NS_OK;
    }
  }
  
  if (mRunsToCompletion) {
    return NS_OK;
  }

  GetParser()->DropStreamParser();

  
  
  
  DidBuildModelImpl(aTerminated || NS_FAILED(IsBroken()));

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
  mDocument->RemoveObserver(this);
  if (!mParser) {
    
    
    return NS_OK;
  }

  
  
  if (mStarted) {
    mDocument->EndLoad();
  }
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
nsHtml5TreeOpExecutor::SetParser(nsParserBase* aParser)
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

nsISupports*
nsHtml5TreeOpExecutor::GetTarget()
{
  return mDocument;
}

nsresult
nsHtml5TreeOpExecutor::MarkAsBroken(nsresult aReason)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  mBroken = aReason;
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
  return aReason;
}

void
FlushTimerCallback(nsITimer* aTimer, void* aClosure)
{
  nsRefPtr<nsHtml5TreeOpExecutor> ex = gBackgroundFlushList->popFirst();
  if (ex) {
    ex->RunFlushLoop();
  }
  if (gBackgroundFlushList && gBackgroundFlushList->isEmpty()) {
    delete gBackgroundFlushList;
    gBackgroundFlushList = nullptr;
    gFlushTimer->Cancel();
    NS_RELEASE(gFlushTimer);
  }
}

void
nsHtml5TreeOpExecutor::ContinueInterruptedParsingAsync()
{
  if (!mDocument || !mDocument->IsInBackgroundWindow()) {
    nsCOMPtr<nsIRunnable> flusher = new nsHtml5ExecutorReflusher(this);  
    if (NS_FAILED(NS_DispatchToMainThread(flusher))) {
      NS_WARNING("failed to dispatch executor flush event");
    }
  } else {
    if (!gBackgroundFlushList) {
      gBackgroundFlushList = new mozilla::LinkedList<nsHtml5TreeOpExecutor>();
    }
    if (!isInList()) {
      gBackgroundFlushList->insertBack(this);
    }
    if (!gFlushTimer) {
      nsCOMPtr<nsITimer> t = do_CreateInstance("@mozilla.org/timer;1");
      t.swap(gFlushTimer);
      
      
      
      gFlushTimer->InitWithFuncCallback(FlushTimerCallback, nullptr,
                                        50, nsITimer::TYPE_REPEATING_SLACK);
    }
  }
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
    if (MOZ_UNLIKELY(!mParser)) {
      
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
    uint32_t mStartTime;
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
        uint32_t timeOffTheEventLoop = 
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
  PROFILER_LABEL("nsHtml5TreeOpExecutor", "RunFlushLoop",
    js::ProfileEntry::Category::OTHER);

  if (mRunFlushLoopOnStack) {
    
    return;
  }
  
  nsHtml5FlushLoopGuard guard(this); 
  
  nsCOMPtr<nsISupports> parserKungFuDeathGrip(mParser);

  
  (void) nsContentSink::WillParseImpl();

  for (;;) {
    if (!mParser) {
      
      mOpQueue.Clear(); 
      return;
    }

    if (NS_FAILED(IsBroken())) {
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
        if (MOZ_UNLIKELY(!mParser)) {
          
          mOpQueue.Clear(); 
          return;
        }
      }
    } else {
      FlushSpeculativeLoads(); 
                               
                               
      if (MOZ_UNLIKELY(!mParser)) {
        
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

    nsIContent* scriptElement = nullptr;
    
    BeginDocUpdate();

    uint32_t numberOfOpsToFlush = mOpQueue.Length();

    const nsHtml5TreeOperation* first = mOpQueue.Elements();
    const nsHtml5TreeOperation* last = first + numberOfOpsToFlush - 1;
    for (nsHtml5TreeOperation* iter = const_cast<nsHtml5TreeOperation*>(first);;) {
      if (MOZ_UNLIKELY(!mParser)) {
        
        break;
      }
      NS_ASSERTION(mFlushState == eInDocUpdate, 
        "Tried to perform tree op outside update batch.");
      nsresult rv = iter->Perform(this, &scriptElement);
      if (NS_FAILED(rv)) {
        MarkAsBroken(rv);
        break;
      }

      
      if (MOZ_UNLIKELY(iter == last)) {
        break;
      } else if (MOZ_UNLIKELY(nsContentSink::DidProcessATokenImpl() == 
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

    if (MOZ_UNLIKELY(!mParser)) {
      
      return;
    }

    if (scriptElement) {
      
      RunScript(scriptElement);
      
      
      StopDeflecting();
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
                

  if (MOZ_UNLIKELY(!mParser)) {
    
    mOpQueue.Clear(); 
    return;
  }
  
  if (mFlushState != eNotFlushing) {
    
    return;
  }

  mFlushState = eInFlush;

  
  nsRefPtr<nsHtml5TreeOpExecutor> kungFuDeathGrip(this);
  nsRefPtr<nsParserBase> parserKungFuDeathGrip(mParser);

  NS_ASSERTION(!mReadingFromStage,
    "Got doc write flush when reading from stage");

#ifdef DEBUG
  mStage.AssertEmpty();
#endif
  
  nsIContent* scriptElement = nullptr;
  
  BeginDocUpdate();

  uint32_t numberOfOpsToFlush = mOpQueue.Length();

  const nsHtml5TreeOperation* start = mOpQueue.Elements();
  const nsHtml5TreeOperation* end = start + numberOfOpsToFlush;
  for (nsHtml5TreeOperation* iter = const_cast<nsHtml5TreeOperation*>(start);
       iter < end;
       ++iter) {
    if (MOZ_UNLIKELY(!mParser)) {
      
      break;
    }
    NS_ASSERTION(mFlushState == eInDocUpdate, 
      "Tried to perform tree op outside update batch.");
    nsresult rv = iter->Perform(this, &scriptElement);
    if (NS_FAILED(rv)) {
      MarkAsBroken(rv);
      break;
    }
  }

  mOpQueue.Clear();
  
  EndDocUpdate();

  mFlushState = eNotFlushing;

  if (MOZ_UNLIKELY(!mParser)) {
    
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
  nsCOMPtr<nsIScriptGlobalObject> globalObject = do_QueryInterface(mDocument->GetInnerWindow());
  
  
  if (!globalObject) {
    globalObject = mDocShell->GetScriptGlobalObject();
    NS_ENSURE_TRUE(globalObject, true);
  }
  NS_ENSURE_TRUE(globalObject && globalObject->GetGlobalJSObject(), true);
  return nsContentUtils::GetSecurityManager()->
           ScriptAllowed(globalObject->GetGlobalJSObject());
}

void
nsHtml5TreeOpExecutor::StartLayout() {
  if (mLayoutStarted || !mDocument) {
    return;
  }

  EndDocUpdate();

  if (MOZ_UNLIKELY(!mParser)) {
    
    return;
  }

  nsContentSink::StartLayout(false);

  BeginDocUpdate();
}












void
nsHtml5TreeOpExecutor::RunScript(nsIContent* aScriptElement)
{
  if (mRunsToCompletion) {
    
    
    
    return;
  }

  NS_ASSERTION(aScriptElement, "No script to run");
  nsCOMPtr<nsIScriptElement> sele = do_QueryInterface(aScriptElement);
  
  if (!mParser) {
    NS_ASSERTION(sele->IsMalformed(), "Script wasn't marked as malformed.");
    
    
    
    return;
  }
  
  if (sele->GetScriptDeferred() || sele->GetScriptAsync()) {
    DebugOnly<bool> block = sele->AttemptToExecute();
    NS_ASSERTION(!block, "Defer or async script tried to block.");
    return;
  }
  
  NS_ASSERTION(mFlushState == eNotFlushing, "Tried to run script when flushing.");

  mReadingFromStage = false;
  
  sele->SetCreatorParser(GetParser());

  
  
  
  bool block = sele->AttemptToExecute();

  
  
  if (block) {
    if (mParser) {
      GetParser()->BlockParser();
    }
  } else {
    

    
    
    nsHtml5TreeOpExecutor::ContinueInterruptedParsingAsync();
  }
}

void
nsHtml5TreeOpExecutor::Start()
{
  NS_PRECONDITION(!mStarted, "Tried to start when already started.");
  mStarted = true;
}

void
nsHtml5TreeOpExecutor::NeedsCharsetSwitchTo(const char* aEncoding,
                                            int32_t aSource,
                                            uint32_t aLineNumber)
{
  EndDocUpdate();

  if (MOZ_UNLIKELY(!mParser)) {
    
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
    
    if (aSource == kCharsetFromMetaTag) {
      MaybeComplainAboutCharset("EncLateMetaReload", false, aLineNumber);
    }
    return;
  }

  if (aSource == kCharsetFromMetaTag) {
    MaybeComplainAboutCharset("EncLateMetaTooLate", true, aLineNumber);
  }

  GetParser()->ContinueAfterFailedCharsetSwitch();

  BeginDocUpdate();
}

void
nsHtml5TreeOpExecutor::MaybeComplainAboutCharset(const char* aMsgId,
                                                 bool aError,
                                                 uint32_t aLineNumber)
{
  if (mAlreadyComplainedAboutCharset) {
    return;
  }
  
  
  
  
  
  
  
  
  
  if (!strcmp(aMsgId, "EncNoDeclaration") && mDocShell) {
    nsCOMPtr<nsIDocShellTreeItem> parent;
    mDocShell->GetSameTypeParent(getter_AddRefs(parent));
    if (parent) {
      return;
    }
  }
  mAlreadyComplainedAboutCharset = true;
  nsContentUtils::ReportToConsole(aError ? nsIScriptError::errorFlag
                                         : nsIScriptError::warningFlag,
                                  NS_LITERAL_CSTRING("HTML parser"),
                                  mDocument,
                                  nsContentUtils::eHTMLPARSER_PROPERTIES,
                                  aMsgId,
                                  nullptr,
                                  0,
                                  nullptr,
                                  EmptyString(),
                                  aLineNumber);
}

void
nsHtml5TreeOpExecutor::ComplainAboutBogusProtocolCharset(nsIDocument* aDoc)
{
  NS_ASSERTION(!mAlreadyComplainedAboutCharset,
               "How come we already managed to complain?");
  mAlreadyComplainedAboutCharset = true;
  nsContentUtils::ReportToConsole(nsIScriptError::errorFlag,
                                  NS_LITERAL_CSTRING("HTML parser"),
                                  aDoc,
                                  nsContentUtils::eHTMLPARSER_PROPERTIES,
                                  "EncProtocolUnsupported");
}

nsHtml5Parser*
nsHtml5TreeOpExecutor::GetParser()
{
  MOZ_ASSERT(!mRunsToCompletion);
  return static_cast<nsHtml5Parser*>(mParser.get());
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
nsHtml5TreeOpExecutor::InitializeDocWriteParserState(nsAHtml5TreeBuilderState* aState, int32_t aLine)
{
  GetParser()->InitializeDocWriteParserState(aState, aLine);
}

nsIURI*
nsHtml5TreeOpExecutor::GetViewSourceBaseURI()
{
  if (!mViewSourceBaseURI) {

    
    
    
    nsCOMPtr<nsIViewSourceChannel> vsc;
    vsc = do_QueryInterface(mDocument->GetChannel());
    if (vsc) {
      nsresult rv =  vsc->GetBaseURI(getter_AddRefs(mViewSourceBaseURI));
      if (NS_SUCCEEDED(rv) && mViewSourceBaseURI) {
        return mViewSourceBaseURI;
      }
    }

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


void
nsHtml5TreeOpExecutor::InitializeStatics()
{
  mozilla::Preferences::AddBoolVarCache(&sExternalViewSource,
                                        "view_source.editor.external");
}

bool
nsHtml5TreeOpExecutor::IsExternalViewSource()
{
  if (!sExternalViewSource) {
    return false;
  }
  bool isViewSource = false;
  if (mDocumentURI) {
    mDocumentURI->SchemeIs("view-source", &isViewSource);
  }
  return isViewSource;
}



already_AddRefed<nsIURI>
nsHtml5TreeOpExecutor::ConvertIfNotPreloadedYet(const nsAString& aURL)
{
  if (aURL.IsEmpty()) {
    return nullptr;
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
    return nullptr;
  }
  nsAutoCString spec;
  uri->GetSpec(spec);
  if (mPreloadedURLs.Contains(spec)) {
    return nullptr;
  }
  mPreloadedURLs.PutEntry(spec);
  return uri.forget();
}

void
nsHtml5TreeOpExecutor::PreloadScript(const nsAString& aURL,
                                     const nsAString& aCharset,
                                     const nsAString& aType,
                                     const nsAString& aCrossOrigin,
                                     bool aScriptFromHead)
{
  nsCOMPtr<nsIURI> uri = ConvertIfNotPreloadedYet(aURL);
  if (!uri) {
    return;
  }
  mDocument->ScriptLoader()->PreloadURI(uri, aCharset, aType, aCrossOrigin,
                                        aScriptFromHead);
}

void
nsHtml5TreeOpExecutor::PreloadStyle(const nsAString& aURL,
                                    const nsAString& aCharset,
                                    const nsAString& aCrossOrigin)
{
  nsCOMPtr<nsIURI> uri = ConvertIfNotPreloadedYet(aURL);
  if (!uri) {
    return;
  }
  mDocument->PreloadStyle(uri, aCharset, aCrossOrigin);
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
uint32_t nsHtml5TreeOpExecutor::sAppendBatchMaxSize = 0;
uint32_t nsHtml5TreeOpExecutor::sAppendBatchSlotsExamined = 0;
uint32_t nsHtml5TreeOpExecutor::sAppendBatchExaminations = 0;
uint32_t nsHtml5TreeOpExecutor::sLongestTimeOffTheEventLoop = 0;
uint32_t nsHtml5TreeOpExecutor::sTimesFlushLoopInterrupted = 0;
#endif
bool nsHtml5TreeOpExecutor::sExternalViewSource = false;
