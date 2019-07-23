







































#include "nsCompatibility.h"
#include "nsScriptLoader.h"
#include "nsNetUtil.h"
#include "nsIStyleSheetLinkingElement.h"
#include "nsICharsetConverterManager.h"
#include "nsICharsetAlias.h"
#include "nsIWebShellServices.h"
#include "nsIDocShell.h"
#include "nsEncoderDecoderUtils.h"
#include "nsContentUtils.h"
#include "nsICharsetDetector.h"
#include "nsIScriptElement.h"
#include "nsIMarkupDocumentViewer.h"
#include "nsIDocShellTreeItem.h"
#include "nsIContentViewer.h"
#include "nsIScriptGlobalObjectOwner.h"
#include "nsIScriptSecurityManager.h"
#include "nsHtml5DocumentMode.h"
#include "nsHtml5Tokenizer.h"
#include "nsHtml5UTF16Buffer.h"
#include "nsHtml5TreeBuilder.h"
#include "nsHtml5Parser.h"

static NS_DEFINE_CID(kCharsetAliasCID, NS_CHARSETALIAS_CID);








class nsHtml5ParserContinueEvent : public nsRunnable
{
public:
  nsRefPtr<nsHtml5Parser> mParser;
  nsHtml5ParserContinueEvent(nsHtml5Parser* aParser)
    : mParser(aParser)
  {}
  NS_IMETHODIMP Run()
  {
    mParser->HandleParserContinueEvent(this);
    return NS_OK;
  }
};


NS_IMPL_CYCLE_COLLECTION_CLASS(nsHtml5Parser)

NS_INTERFACE_TABLE_HEAD_CYCLE_COLLECTION_INHERITED(nsHtml5Parser) \
  NS_INTERFACE_TABLE_INHERITED4(nsHtml5Parser, 
                                nsIParser, 
                                nsIStreamListener, 
                                nsICharsetDetectionObserver, 
                                nsIContentSink)
NS_INTERFACE_TABLE_TAIL_INHERITING(nsContentSink)

NS_IMPL_ADDREF_INHERITED(nsHtml5Parser, nsContentSink)

NS_IMPL_RELEASE_INHERITED(nsHtml5Parser, nsContentSink)

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(nsHtml5Parser, nsContentSink)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mScriptElement)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mRequest)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mObserver)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mUnicodeDecoder)
  tmp->mTreeBuilder->DoTraverse(cb);
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(nsHtml5Parser, nsContentSink)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mScriptElement)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mRequest)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mObserver)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mUnicodeDecoder)
  tmp->mTreeBuilder->DoUnlink();
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

nsHtml5Parser::nsHtml5Parser()
  : mFirstBuffer(new nsHtml5UTF16Buffer(NS_HTML5_PARSER_READ_BUFFER_SIZE)),
    
    mLastBuffer(mFirstBuffer),
    mTreeBuilder(new nsHtml5TreeBuilder(this)),
    mTokenizer(new nsHtml5Tokenizer(mTreeBuilder))
{
  mTokenizer->setEncodingDeclarationHandler(this);
  
}

nsHtml5Parser::~nsHtml5Parser()
{
  while (mFirstBuffer) {
     nsHtml5UTF16Buffer* old = mFirstBuffer;
     mFirstBuffer = mFirstBuffer->next;
     delete old;
  }
#ifdef GATHER_DOCWRITE_STATISTICS
  delete mSnapshot;
#endif
}


static PRBool
IsScriptEnabled(nsIDocument *aDoc, nsIDocShell *aContainer)
{
  NS_ENSURE_TRUE(aDoc && aContainer, PR_TRUE);
  nsCOMPtr<nsIScriptGlobalObject> globalObject = aDoc->GetScriptGlobalObject();
  
  
  if (!globalObject) {
    nsCOMPtr<nsIScriptGlobalObjectOwner> owner = do_GetInterface(aContainer);
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
    CanExecuteScripts(cx, aDoc->NodePrincipal(), &enabled);
  return enabled;
}

NS_IMETHODIMP_(void)
nsHtml5Parser::SetContentSink(nsIContentSink* aSink)
{
  NS_ASSERTION(aSink == static_cast<nsIContentSink*> (this), 
               "Attempt to set a foreign sink.");
}

NS_IMETHODIMP_(nsIContentSink*)
nsHtml5Parser::GetContentSink(void)
{
  return static_cast<nsIContentSink*> (this);
}

NS_IMETHODIMP_(void)
nsHtml5Parser::GetCommand(nsCString& aCommand)
{
  aCommand.Assign("view");
}

NS_IMETHODIMP_(void)
nsHtml5Parser::SetCommand(const char* aCommand)
{
  NS_ASSERTION(!strcmp(aCommand, "view"), "Parser command was not view");
}

NS_IMETHODIMP_(void)
nsHtml5Parser::SetCommand(eParserCommands aParserCommand)
{
  NS_ASSERTION(aParserCommand == eViewNormal, 
               "Parser command was not eViewNormal.");
}

NS_IMETHODIMP_(void)
nsHtml5Parser::SetDocumentCharset(const nsACString& aCharset, PRInt32 aCharsetSource)
{
  mCharset = aCharset;
  mCharsetSource = aCharsetSource;
}

NS_IMETHODIMP_(void)
nsHtml5Parser::SetParserFilter(nsIParserFilter* aFilter)
{
  NS_ERROR("Attempt to set a parser filter on HTML5 parser.");
}

NS_IMETHODIMP
nsHtml5Parser::GetChannel(nsIChannel** aChannel)
{
  return mRequest ? CallQueryInterface(mRequest, aChannel) :
                    NS_ERROR_NOT_AVAILABLE;
}

NS_IMETHODIMP
nsHtml5Parser::GetDTD(nsIDTD** aDTD)
{
  *aDTD = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
nsHtml5Parser::ContinueParsing()
{
  mBlocked = PR_FALSE;
  return ContinueInterruptedParsing();
}

NS_IMETHODIMP
nsHtml5Parser::ContinueInterruptedParsing()
{
  
  
  
  if (IsScriptExecutingImpl()) {
    return NS_OK;
  }
  
  
  
  
  
  nsCOMPtr<nsIParser> kungFuDeathGrip(this);
  
  mTreeBuilder->MaybeFlush();
  ParseUntilSuspend();
  return NS_OK;
}

NS_IMETHODIMP_(void)
nsHtml5Parser::BlockParser()
{
  NS_NOTREACHED("No one should call this");
}

NS_IMETHODIMP_(void)
nsHtml5Parser::UnblockParser()
{
  mBlocked = PR_FALSE;
}

NS_IMETHODIMP_(PRBool)
nsHtml5Parser::IsParserEnabled()
{
  return !mBlocked;
}

NS_IMETHODIMP_(PRBool)
nsHtml5Parser::IsComplete()
{
  
  
  return (mLifeCycle == TERMINATED);
}

NS_IMETHODIMP
nsHtml5Parser::Parse(nsIURI* aURL, 
                     nsIRequestObserver* aObserver,
                     void* aKey,
                     nsDTDMode aMode) 
{
  mObserver = aObserver;
  mRootContextKey = aKey;
  mCanInterruptParser = PR_TRUE;
  NS_PRECONDITION(mLifeCycle == NOT_STARTED, 
                  "Tried to start parse without initializing the parser properly.");
  return NS_OK;
}

NS_IMETHODIMP
nsHtml5Parser::Parse(const nsAString& aSourceBuffer,
                     void* aKey,
                     const nsACString& aContentType, 
                     PRBool aLastCall,
                     nsDTDMode aMode) 
{
  NS_PRECONDITION(!mFragmentMode, "Document.write called in fragment mode!");
  
  switch (mLifeCycle) {
    case TERMINATED:
      return NS_OK;
    case NOT_STARTED:
      mTreeBuilder->setScriptingEnabled(IsScriptEnabled(mDocument, mDocShell));
      mTokenizer->start();
      mLifeCycle = PARSING;
      mParser = this;
      mCharsetSource = kCharsetFromOtherComponent;
      break;
    default:
      break;
  }

  if (aLastCall && aSourceBuffer.IsEmpty() && aKey == GetRootContextKey()) {
    
    mLifeCycle = STREAM_ENDING;
    MaybePostContinueEvent();
    return NS_OK;
  }

  

  PRInt32 lineNumberSave = mTokenizer->getLineNumber();

  
  
  
  nsCOMPtr<nsIParser> kungFuDeathGrip(this);

  if (!aSourceBuffer.IsEmpty()) {
    nsHtml5UTF16Buffer* buffer = new nsHtml5UTF16Buffer(aSourceBuffer.Length());
    memcpy(buffer->getBuffer(), aSourceBuffer.BeginReading(), aSourceBuffer.Length() * sizeof(PRUnichar));
    buffer->setEnd(aSourceBuffer.Length());
    if (!mBlocked) {
      WillResumeImpl();
      WillParseImpl();
      while (buffer->hasMore()) {
        buffer->adjust(mLastWasCR);
        mLastWasCR = PR_FALSE;
        if (buffer->hasMore()) {
          mUninterruptibleDocWrite = PR_TRUE;
          mLastWasCR = mTokenizer->tokenizeBuffer(buffer);
          if (mScriptElement) {
            mTreeBuilder->Flush();
            mUninterruptibleDocWrite = PR_FALSE;
            ExecuteScript();
          }
          if (mBlocked) {
            
            WillInterruptImpl();
            break;
          }
          
        }
      }
    }

    mUninterruptibleDocWrite = PR_FALSE;

    if (buffer->hasMore()) {
      
      
      
      
      
      
      
      
      nsHtml5UTF16Buffer* prevSearchBuf = nsnull;
      nsHtml5UTF16Buffer* searchBuf = mFirstBuffer;
      if (aKey) { 
        while (searchBuf != mLastBuffer) {
          if (searchBuf->key == aKey) {
            
            
            
            buffer->next = searchBuf;
            if (prevSearchBuf) {
              prevSearchBuf->next = buffer;
            } else {
              mFirstBuffer = buffer;
            }
            break;
          }
          prevSearchBuf = searchBuf;
          searchBuf = searchBuf->next;
        }
      }
      if (searchBuf == mLastBuffer || !aKey) {
        
        
        nsHtml5UTF16Buffer* keyHolder = new nsHtml5UTF16Buffer(aKey);
        keyHolder->next = mFirstBuffer;
        buffer->next = keyHolder;
        mFirstBuffer = buffer;
      }
      MaybePostContinueEvent();
    } else {
      delete buffer;
    }
  }

  
  mTreeBuilder->Flush();
  mTokenizer->setLineNumber(lineNumberSave);
  return NS_OK;
}




NS_IMETHODIMP_(void *)
nsHtml5Parser::GetRootContextKey()
{
  return mRootContextKey;
}

NS_IMETHODIMP
nsHtml5Parser::Terminate(void)
{
  
  
  if (mLifeCycle == TERMINATED) {
    return NS_OK;
  }
  
  
  nsCOMPtr<nsIParser> kungFuDeathGrip(this);
  
  
  CancelParsingEvents();
#ifdef DEBUG
  PRBool ready =
#endif
  ReadyToCallDidBuildModelImpl(PR_TRUE);
  NS_ASSERTION(ready, "Should always be ready to call DidBuildModel here.");
  return DidBuildModel();
}

NS_IMETHODIMP
nsHtml5Parser::ParseFragment(const nsAString& aSourceBuffer,
                             void* aKey,
                             nsTArray<nsString>& aTagStack,
                             PRBool aXMLMode,
                             const nsACString& aContentType,
                             nsDTDMode aMode)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsHtml5Parser::ParseFragment(const nsAString& aSourceBuffer,
                             nsISupports* aTargetNode,
                             nsIAtom* aContextLocalName,
                             PRInt32 aContextNamespace,
                             PRBool aQuirks)
{
  nsCOMPtr<nsIContent> target = do_QueryInterface(aTargetNode);
  NS_ASSERTION(target, "Target did not QI to nsIContent");

  nsIDocument* doc = target->GetOwnerDoc();
  NS_ENSURE_TRUE(doc, NS_ERROR_NOT_AVAILABLE);
  
  nsIURI* uri = doc->GetDocumentURI();
  NS_ENSURE_TRUE(uri, NS_ERROR_NOT_AVAILABLE);

  Initialize(doc, uri, nsnull, nsnull);

  
  mDocumentBaseURI = doc->GetBaseURI();

  mTreeBuilder->setFragmentContext(aContextLocalName, aContextNamespace, target, aQuirks);
  mFragmentMode = PR_TRUE;
  mCanInterruptParser = PR_FALSE;
  NS_PRECONDITION(mLifeCycle == NOT_STARTED, "Tried to start parse without initializing the parser properly.");
  mTreeBuilder->setScriptingEnabled(IsScriptEnabled(mDocument, mDocShell));
  mTokenizer->start();
  mLifeCycle = PARSING;
  mParser = this;
  mNodeInfoManager = target->GetOwnerDoc()->NodeInfoManager();
  if (!aSourceBuffer.IsEmpty()) {
    PRBool lastWasCR = PR_FALSE;
    nsHtml5UTF16Buffer buffer(aSourceBuffer.Length());
    memcpy(buffer.getBuffer(), aSourceBuffer.BeginReading(), aSourceBuffer.Length() * sizeof(PRUnichar));
    buffer.setEnd(aSourceBuffer.Length());
    while (buffer.hasMore()) {
      buffer.adjust(lastWasCR);
      lastWasCR = PR_FALSE;
      if (buffer.hasMore()) {
        lastWasCR = mTokenizer->tokenizeBuffer(&buffer);
        if (mScriptElement) {
          nsCOMPtr<nsIScriptElement> script = do_QueryInterface(mScriptElement);
          NS_ASSERTION(script, "mScriptElement didn't QI to nsIScriptElement!");
          script->PreventExecution();
          mScriptElement = nsnull;
        }
      }
    }
  }
  mLifeCycle = TERMINATED;
  mTokenizer->eof();
  mTokenizer->end();
  mTreeBuilder->Flush();
  DropParserAndPerfHint();
  return NS_OK;
}

NS_IMETHODIMP
nsHtml5Parser::BuildModel(void)
{
  
  ParseUntilSuspend();
  return NS_OK;
}

NS_IMETHODIMP
nsHtml5Parser::CancelParsingEvents()
{
  mContinueEvent = nsnull;
  return NS_OK;
}

void
nsHtml5Parser::Reset()
{
  mNeedsCharsetSwitch = PR_FALSE;
  mLastWasCR = PR_FALSE;
  mFragmentMode = PR_FALSE;
  mBlocked = PR_FALSE;
  mSuspending = PR_FALSE;
  mLifeCycle = NOT_STARTED;
  mScriptElement = nsnull;
  mUninterruptibleDocWrite = PR_FALSE;
  mRootContextKey = nsnull;
  mRequest = nsnull;
  mObserver = nsnull;
  mContinueEvent = nsnull;  
  
  mCharsetSource = kCharsetUninitialized;
  mCharset.Truncate();
  mPendingCharset.Truncate();
  mUnicodeDecoder = nsnull;
  mSniffingBuffer = nsnull;
  mSniffingLength = 0;
  mBomState = BOM_SNIFFING_NOT_STARTED;
  mMetaScanner = nsnull;
  
  while (mFirstBuffer->next) {
    nsHtml5UTF16Buffer* oldBuf = mFirstBuffer;
    mFirstBuffer = mFirstBuffer->next;
    delete oldBuf;
  }
  mFirstBuffer->setStart(0);
  mFirstBuffer->setEnd(0);
#ifdef DEBUG
  mStreamListenerState = eNone;
#endif
}

PRBool
nsHtml5Parser::CanInterrupt()
{
  return !(mFragmentMode || mUninterruptibleDocWrite);
}




nsresult
nsHtml5Parser::OnStartRequest(nsIRequest* aRequest, nsISupports* aContext)
{
  NS_PRECONDITION(eNone == mStreamListenerState,
                  "Parser's nsIStreamListener API was not setup "
                  "correctly in constructor.");
  if (mObserver) {
    mObserver->OnStartRequest(aRequest, aContext);
  }
#ifdef DEBUG
  mStreamListenerState = eOnStart;
#endif
  mRequest = aRequest;
  
  if (mCharsetSource < kCharsetFromChannel) {
    
    
    return NS_OK;
  }
  
  nsresult rv = NS_OK;
  nsCOMPtr<nsICharsetConverterManager> convManager = do_GetService(NS_CHARSETCONVERTERMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = convManager->GetUnicodeDecoder(mCharset.get(), getter_AddRefs(mUnicodeDecoder));
  NS_ENSURE_SUCCESS(rv, rv);
  mUnicodeDecoder->SetInputErrorBehavior(nsIUnicodeDecoder::kOnError_Recover);
  return NS_OK;
}

nsresult
nsHtml5Parser::OnStopRequest(nsIRequest* aRequest,
                             nsISupports* aContext,
                             nsresult status)
{
  mTreeBuilder->MaybeFlush();
  NS_ASSERTION(mRequest == aRequest, "Got Stop on wrong stream.");
  nsresult rv = NS_OK;
  if (!mUnicodeDecoder) {
    PRUint32 writeCount;
    rv = FinalizeSniffing(nsnull, 0, &writeCount, 0);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  switch (mLifeCycle) {
    case TERMINATED:
      break;
    case NOT_STARTED:
      mTreeBuilder->setScriptingEnabled(IsScriptEnabled(mDocument, mDocShell));
      mTokenizer->start();
      mLifeCycle = STREAM_ENDING;
      mParser = this;
      break;
    case STREAM_ENDING:
      NS_ERROR("OnStopRequest when the stream lifecycle was already ending.");
      break;
    default:
      mLifeCycle = STREAM_ENDING;
      break;
  }
#ifdef DEBUG
  mStreamListenerState = eOnStop;
#endif
  if (!IsScriptExecutingImpl()) {
    ParseUntilSuspend();
  }
  if (mObserver) {
    mObserver->OnStopRequest(aRequest, aContext, status);
  }
  return NS_OK;
}








static NS_METHOD
ParserWriteFunc(nsIInputStream* aInStream,
                void* aHtml5Parser,
                const char* aFromSegment,
                PRUint32 aToOffset,
                PRUint32 aCount,
                PRUint32* aWriteCount)
{
  nsHtml5Parser* parser = static_cast<nsHtml5Parser*> (aHtml5Parser);
  if (parser->HasDecoder()) {
    return parser->WriteStreamBytes((const PRUint8*)aFromSegment, aCount, aWriteCount);
  } else {
    return parser->SniffStreamBytes((const PRUint8*)aFromSegment, aCount, aWriteCount);
  }
}

nsresult
nsHtml5Parser::OnDataAvailable(nsIRequest* aRequest,
                               nsISupports* aContext,
                               nsIInputStream* aInStream,
                               PRUint32 aSourceOffset,
                               PRUint32 aLength)
{
  mTreeBuilder->MaybeFlush();
  NS_PRECONDITION(eOnStart == mStreamListenerState ||
                  eOnDataAvail == mStreamListenerState,
            "Error: OnStartRequest() must be called before OnDataAvailable()");
  NS_ASSERTION(mRequest == aRequest, "Got data on wrong stream.");
  PRUint32 totalRead;
  nsresult rv = aInStream->ReadSegments(ParserWriteFunc, static_cast<void*> (this), aLength, &totalRead);
  NS_ASSERTION(totalRead == aLength, "ReadSegments read the wrong number of bytes.");
  if (!IsScriptExecutingImpl()) {
    ParseUntilSuspend();
  }
  return rv;
}


void
nsHtml5Parser::internalEncodingDeclaration(nsString* aEncoding)
{
  if (mCharsetSource >= kCharsetFromMetaTag) { 
    return;
  }
  nsresult rv = NS_OK;
  nsCOMPtr<nsICharsetAlias> calias(do_GetService(kCharsetAliasCID, &rv));
  if (NS_FAILED(rv)) {
    return;
  }
  nsCAutoString newEncoding;
  CopyUTF16toUTF8(*aEncoding, newEncoding);
  PRBool eq;
  rv = calias->Equals(newEncoding, mCharset, &eq);
  if (NS_FAILED(rv)) {
    return;
  }
  if (eq) {
    mCharsetSource = kCharsetFromMetaTag; 
    return;
  }
  
  

  
  nsCOMPtr<nsIHttpChannel> httpChannel(do_QueryInterface(mRequest, &rv));
  if (NS_SUCCEEDED(rv)) {
    nsCAutoString method;
    httpChannel->GetRequestMethod(method);
    
    
    if (!method.EqualsLiteral("GET")) {
      
      
      return;
    }
  }
  
  
  mNeedsCharsetSwitch = PR_TRUE;
  mPendingCharset.Assign(newEncoding);
}


void
nsHtml5Parser::documentMode(nsHtml5DocumentMode m)
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


NS_IMETHODIMP
nsHtml5Parser::WillParse()
{
  NS_NOTREACHED("No one should call this");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsHtml5Parser::WillBuildModel(nsDTDMode aDTDMode)
{
  NS_NOTREACHED("No one should call this");
  return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
nsHtml5Parser::DidBuildModel()
{
  NS_ASSERTION(mLifeCycle == STREAM_ENDING, "Bad life cycle.");
  mLifeCycle = TERMINATED;
  mTokenizer->eof();
  mTokenizer->end();
  mTreeBuilder->Flush();
  
  DidBuildModelImpl();
  mDocument->ScriptLoader()->RemoveObserver(this);
  StartLayout(PR_FALSE);
  ScrollToRef();
  mDocument->RemoveObserver(this);
  mDocument->EndLoad();
  DropParserAndPerfHint();
#ifdef GATHER_DOCWRITE_STATISTICS
  printf("UNSAFE SCRIPTS: %d\n", sUnsafeDocWrites);
  printf("TOKENIZER-SAFE SCRIPTS: %d\n", sTokenSafeDocWrites);
  printf("TREEBUILDER-SAFE SCRIPTS: %d\n", sTreeSafeDocWrites);
#endif
  return NS_OK;
}

NS_IMETHODIMP
nsHtml5Parser::WillInterrupt()
{
  return WillInterruptImpl();
}

NS_IMETHODIMP
nsHtml5Parser::WillResume()
{
  NS_NOTREACHED("No one should call this.");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsHtml5Parser::SetParser(nsIParser* aParser)
{
  NS_NOTREACHED("No one should be setting a parser on the HTML5 pseudosink.");
  return NS_ERROR_NOT_IMPLEMENTED;
}

void
nsHtml5Parser::FlushPendingNotifications(mozFlushType aType)
{
}

NS_IMETHODIMP
nsHtml5Parser::SetDocumentCharset(nsACString& aCharset)
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
nsHtml5Parser::GetTarget()
{
  return mDocument;
}


void
nsHtml5Parser::HandleParserContinueEvent(nsHtml5ParserContinueEvent* ev)
{
  
  if (mContinueEvent != ev)
    return;
  mContinueEvent = nsnull;
  NS_ASSERTION(!IsScriptExecutingImpl(), "Interrupted in the middle of a script?");
  ContinueInterruptedParsing();
}

NS_IMETHODIMP
nsHtml5Parser::Notify(const char* aCharset, nsDetectionConfident aConf)
{
  if (aConf == eBestAnswer || aConf == eSureAnswer) {
    mCharset.Assign(aCharset);
    mCharsetSource = kCharsetFromAutoDetection;
    SetDocumentCharset(mCharset);
  }
  return NS_OK;
}

nsresult
nsHtml5Parser::SetupDecodingAndWriteSniffingBufferAndCurrentSegment(const PRUint8* aFromSegment, 
                                                                    PRUint32 aCount,
                                                                    PRUint32* aWriteCount)
{
  nsresult rv = NS_OK;
  nsCOMPtr<nsICharsetConverterManager> convManager = do_GetService(NS_CHARSETCONVERTERMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = convManager->GetUnicodeDecoder(mCharset.get(), getter_AddRefs(mUnicodeDecoder));
  if (rv == NS_ERROR_UCONV_NOCONV) {
    mCharset.Assign("windows-1252"); 
    mCharsetSource = kCharsetFromWeakDocTypeDefault;
    rv = convManager->GetUnicodeDecoderRaw(mCharset.get(), getter_AddRefs(mUnicodeDecoder));
    SetDocumentCharset(mCharset);
  }
  NS_ENSURE_SUCCESS(rv, rv);
  mUnicodeDecoder->SetInputErrorBehavior(nsIUnicodeDecoder::kOnError_Recover);
  return WriteSniffingBufferAndCurrentSegment(aFromSegment, aCount, aWriteCount);
}

nsresult
nsHtml5Parser::WriteSniffingBufferAndCurrentSegment(const PRUint8* aFromSegment, 
                                                    PRUint32 aCount,
                                                    PRUint32* aWriteCount)
{
  nsresult rv = NS_OK;
  if (mSniffingBuffer) {
    PRUint32 writeCount;
    rv = WriteStreamBytes(mSniffingBuffer, mSniffingLength, &writeCount);
    NS_ENSURE_SUCCESS(rv, rv);
    mSniffingBuffer = nsnull;
  }
  mMetaScanner = nsnull;
  if (aFromSegment) {
    rv = WriteStreamBytes(aFromSegment, aCount, aWriteCount);
  }
  return rv;
}

nsresult
nsHtml5Parser::SetupDecodingFromBom(const char* aCharsetName, const char* aDecoderCharsetName)
{
  nsresult rv = NS_OK;
  nsCOMPtr<nsICharsetConverterManager> convManager = do_GetService(NS_CHARSETCONVERTERMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = convManager->GetUnicodeDecoderRaw(aDecoderCharsetName, getter_AddRefs(mUnicodeDecoder));
  NS_ENSURE_SUCCESS(rv, rv);
  mCharset.Assign(aCharsetName);
  mCharsetSource = kCharsetFromByteOrderMark;
  SetDocumentCharset(mCharset);
  mSniffingBuffer = nsnull;
  mMetaScanner = nsnull;
  mBomState = BOM_SNIFFING_OVER;
  return rv;
}

nsresult
nsHtml5Parser::FinalizeSniffing(const PRUint8* aFromSegment, 
                                PRUint32 aCount,
                                PRUint32* aWriteCount,
                                PRUint32 aCountToSniffingLimit)
{
  
  if (mCharsetSource >= kCharsetFromHintPrevDoc) {
    return SetupDecodingAndWriteSniffingBufferAndCurrentSegment(aFromSegment, aCount, aWriteCount);
  }
  
  const nsAdoptingString& detectorName = nsContentUtils::GetLocalizedStringPref("intl.charset.detector");
  if (!detectorName.IsEmpty()) {
    nsCAutoString detectorContractID;
    detectorContractID.AssignLiteral(NS_CHARSET_DETECTOR_CONTRACTID_BASE);
    AppendUTF16toUTF8(detectorName, detectorContractID);
    nsCOMPtr<nsICharsetDetector> detector = do_CreateInstance(detectorContractID.get());
    if (detector) {
      nsresult rv = detector->Init(this);
      NS_ENSURE_SUCCESS(rv, rv);
      PRBool dontFeed = PR_FALSE;
      if (mSniffingBuffer) {
        rv = detector->DoIt((const char*)mSniffingBuffer.get(), mSniffingLength, &dontFeed);
        NS_ENSURE_SUCCESS(rv, rv);
      }
      if (!dontFeed && aFromSegment) {
        rv = detector->DoIt((const char*)aFromSegment, aCountToSniffingLimit, &dontFeed);
        NS_ENSURE_SUCCESS(rv, rv);
      }
      rv = detector->Done();
      NS_ENSURE_SUCCESS(rv, rv);
      
    } else {
      NS_ERROR("Could not instantiate charset detector.");
    }
  }
  if (mCharsetSource == kCharsetUninitialized) {
    
    mCharset.Assign("windows-1252");
    mCharsetSource = kCharsetFromWeakDocTypeDefault;
    SetDocumentCharset(mCharset);
  }
  return SetupDecodingAndWriteSniffingBufferAndCurrentSegment(aFromSegment, aCount, aWriteCount);
}

nsresult
nsHtml5Parser::SniffStreamBytes(const PRUint8* aFromSegment,
                                PRUint32 aCount,
                                PRUint32* aWriteCount)
{
  nsresult rv = NS_OK;
  PRUint32 writeCount;
  for (PRUint32 i = 0; i < aCount; i++) {
    switch (mBomState) {
      case BOM_SNIFFING_NOT_STARTED:
        NS_ASSERTION(i == 0, "Bad BOM sniffing state.");
        switch (*aFromSegment) {
          case 0xEF:
            mBomState = SEEN_UTF_8_FIRST_BYTE;
            break;
          case 0xFF:
            mBomState = SEEN_UTF_16_LE_FIRST_BYTE;
            break;
          case 0xFE:
            mBomState = SEEN_UTF_16_BE_FIRST_BYTE;
            break;
          default:
            mBomState = BOM_SNIFFING_OVER;
            break;
        }
        break;
      case SEEN_UTF_16_LE_FIRST_BYTE:
        if (aFromSegment[i] == 0xFE) {
          rv = SetupDecodingFromBom("UTF-16", "UTF-16LE"); 
          NS_ENSURE_SUCCESS(rv, rv);
          PRUint32 count = aCount - (i + 1);
          rv = WriteStreamBytes(aFromSegment + (i + 1), count, &writeCount);
          NS_ENSURE_SUCCESS(rv, rv);
          *aWriteCount = writeCount + (i + 1);
          return rv;
        }
        mBomState = BOM_SNIFFING_OVER;
        break;
      case SEEN_UTF_16_BE_FIRST_BYTE:
        if (aFromSegment[i] == 0xFF) {
          rv = SetupDecodingFromBom("UTF-16", "UTF-16BE"); 
          NS_ENSURE_SUCCESS(rv, rv);
          PRUint32 count = aCount - (i + 1);
          rv = WriteStreamBytes(aFromSegment + (i + 1), count, &writeCount);
          NS_ENSURE_SUCCESS(rv, rv);
          *aWriteCount = writeCount + (i + 1);
          return rv;
        }
        mBomState = BOM_SNIFFING_OVER;
        break;
      case SEEN_UTF_8_FIRST_BYTE:
        if (aFromSegment[i] == 0xBB) {
          mBomState = SEEN_UTF_8_SECOND_BYTE;
        } else {
          mBomState = BOM_SNIFFING_OVER;
        }
        break;
      case SEEN_UTF_8_SECOND_BYTE:
        if (aFromSegment[i] == 0xBF) {
          rv = SetupDecodingFromBom("UTF-8", "UTF-8"); 
          NS_ENSURE_SUCCESS(rv, rv);
          PRUint32 count = aCount - (i + 1);
          rv = WriteStreamBytes(aFromSegment + (i + 1), count, &writeCount);
          NS_ENSURE_SUCCESS(rv, rv);
          *aWriteCount = writeCount + (i + 1);
          return rv;
        }
        mBomState = BOM_SNIFFING_OVER;
        break;
      default:
        goto bom_loop_end;
    }
  }
  
  bom_loop_end:
  
  if (!mMetaScanner) {
    mMetaScanner = new nsHtml5MetaScanner();
  }
  
  if (mSniffingLength + aCount >= NS_HTML5_PARSER_SNIFFING_BUFFER_SIZE) {
    
    PRUint32 countToSniffingLimit = NS_HTML5_PARSER_SNIFFING_BUFFER_SIZE - mSniffingLength;
    nsHtml5ByteReadable readable(aFromSegment, aFromSegment + countToSniffingLimit);
    mMetaScanner->sniff(&readable, getter_AddRefs(mUnicodeDecoder), mCharset);
    if (mUnicodeDecoder) {
      mUnicodeDecoder->SetInputErrorBehavior(nsIUnicodeDecoder::kOnError_Recover);
      
      mCharsetSource = kCharsetFromMetaPrescan;
      SetDocumentCharset(mCharset);
      mMetaScanner = nsnull;
      return WriteSniffingBufferAndCurrentSegment(aFromSegment, aCount, aWriteCount);
    }
    return FinalizeSniffing(aFromSegment, aCount, aWriteCount, countToSniffingLimit);
  }

  
  nsHtml5ByteReadable readable(aFromSegment, aFromSegment + aCount);
  mMetaScanner->sniff(&readable, getter_AddRefs(mUnicodeDecoder), mCharset);
  if (mUnicodeDecoder) {
    
    mCharsetSource = kCharsetFromMetaPrescan;
    SetDocumentCharset(mCharset);
    mMetaScanner = nsnull;
    return WriteSniffingBufferAndCurrentSegment(aFromSegment, aCount, aWriteCount);
  }
  if (!mSniffingBuffer) {
    mSniffingBuffer = new PRUint8[NS_HTML5_PARSER_SNIFFING_BUFFER_SIZE];
  }
  memcpy(mSniffingBuffer + mSniffingLength, aFromSegment, aCount);
  mSniffingLength += aCount;
  *aWriteCount = aCount;
  return NS_OK;
}

nsresult
nsHtml5Parser::WriteStreamBytes(const PRUint8* aFromSegment,
                                PRUint32 aCount,
                                PRUint32* aWriteCount)
{
  
  if (mLastBuffer->getEnd() == NS_HTML5_PARSER_READ_BUFFER_SIZE) {
    mLastBuffer = (mLastBuffer->next = new nsHtml5UTF16Buffer(NS_HTML5_PARSER_READ_BUFFER_SIZE));
  }
  PRUint32 totalByteCount = 0;
  for (;;) {
    PRInt32 end = mLastBuffer->getEnd();
    PRInt32 byteCount = aCount - totalByteCount;
    PRInt32 utf16Count = NS_HTML5_PARSER_READ_BUFFER_SIZE - end;

    NS_ASSERTION(utf16Count, "Trying to convert into a buffer with no free space!");

    nsresult convResult = mUnicodeDecoder->Convert((const char*)aFromSegment, &byteCount, mLastBuffer->getBuffer() + end, &utf16Count);

    end += utf16Count;
    mLastBuffer->setEnd(end);
    totalByteCount += byteCount;
    aFromSegment += byteCount;

    NS_ASSERTION(mLastBuffer->getEnd() <= NS_HTML5_PARSER_READ_BUFFER_SIZE, "The Unicode decoder wrote too much data.");

    if (NS_FAILED(convResult)) {
      if (totalByteCount < aCount) { 
        ++totalByteCount;
        ++aFromSegment;
      }
      mLastBuffer->getBuffer()[end] = 0xFFFD;
      ++end;
      mLastBuffer->setEnd(end);
      if (end == NS_HTML5_PARSER_READ_BUFFER_SIZE) {
          mLastBuffer = (mLastBuffer->next = new nsHtml5UTF16Buffer(NS_HTML5_PARSER_READ_BUFFER_SIZE));
      }
      mUnicodeDecoder->Reset();
      if (totalByteCount == aCount) {
        *aWriteCount = totalByteCount;
        return NS_OK;
      }
    } else if (convResult == NS_PARTIAL_MORE_OUTPUT) {
      mLastBuffer = (mLastBuffer->next = new nsHtml5UTF16Buffer(NS_HTML5_PARSER_READ_BUFFER_SIZE));
      NS_ASSERTION(totalByteCount < aCount, "The Unicode decoder has consumed too many bytes.");
    } else {
      NS_ASSERTION(totalByteCount == aCount, "The Unicode decoder consumed the wrong number of bytes.");
      *aWriteCount = totalByteCount;
      return NS_OK;
    }
  }
}

void
nsHtml5Parser::ParseUntilSuspend()
{
  NS_PRECONDITION(!mFragmentMode, "ParseUntilSuspend called in fragment mode.");
  NS_PRECONDITION(!mNeedsCharsetSwitch, "ParseUntilSuspend called when charset switch needed.");

  if (mBlocked) {
    return;
  }

  switch (mLifeCycle) {
    case TERMINATED:
      return;
    case NOT_STARTED:
      mTreeBuilder->setScriptingEnabled(IsScriptEnabled(mDocument, mDocShell));
      mTokenizer->start();
      mLifeCycle = PARSING;
      mParser = this;
      break;
    default:
      break;
  }

  WillResumeImpl();
  WillParseImpl();
  mSuspending = PR_FALSE;
  for (;;) {
    if (!mFirstBuffer->hasMore()) {
      if (mFirstBuffer == mLastBuffer) {
        switch (mLifeCycle) {
          case TERMINATED:
            
            return;
          case PARSING:
            
            mFirstBuffer->setStart(0);
            mFirstBuffer->setEnd(0);
            return; 
          case STREAM_ENDING:
            if (ReadyToCallDidBuildModelImpl(PR_FALSE)) {
              DidBuildModel();
            }
            return; 
          default:
            NS_NOTREACHED("It should be impossible to reach this.");
            return;
        }
      } else {
        nsHtml5UTF16Buffer* oldBuf = mFirstBuffer;
        mFirstBuffer = mFirstBuffer->next;
        delete oldBuf;
        continue;
      }
    }

    if (mBlocked || (mLifeCycle == TERMINATED)) {
      return;
    }

#ifdef GATHER_DOCWRITE_STATISTICS
    if (mSnapshot && mFirstBuffer->key == GetRootContextKey()) {
      if (mTokenizer->isInDataState()) {
        if (mTreeBuilder->snapshotMatches(mSnapshot)) {
          sTreeSafeDocWrites++;
        } else {
          sTokenSafeDocWrites++;
        }
      } else {
        sUnsafeDocWrites++;
      }
      delete mSnapshot;
      mSnapshot = nsnull;
    }
#endif

    
    mFirstBuffer->adjust(mLastWasCR);
    mLastWasCR = PR_FALSE;
    if (mFirstBuffer->hasMore()) {
      mLastWasCR = mTokenizer->tokenizeBuffer(mFirstBuffer);
      NS_ASSERTION(!(mScriptElement && mNeedsCharsetSwitch), "Can't have both script and charset switch.");
      if (mScriptElement) {
        mTreeBuilder->Flush();
        ExecuteScript();
      } else if (mNeedsCharsetSwitch) {
        if (PerformCharsetSwitch() == NS_ERROR_HTMLPARSER_STOPPARSING) {
          return;
        } else {
          
          mNeedsCharsetSwitch = PR_FALSE;
        }
      }
      if (mBlocked) {
        WillInterruptImpl();
        return;
      }
      if (mSuspending) {
        MaybePostContinueEvent();
        WillInterruptImpl();
        return;
      }
    }
    continue;
  }
}

nsresult
nsHtml5Parser::PerformCharsetSwitch()
{
  
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
    rv = NS_ERROR_HTMLPARSER_CONTINUE;
  return rv;
}







void
nsHtml5Parser::ExecuteScript()
{
  NS_PRECONDITION(mScriptElement, "Trying to run a script without having one!");
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
    mBlocked = PR_TRUE;
  } else {
    
    
    htmlDocument->ScriptExecuted(sele);
  }
  mScriptElement = nsnull;
}

void
nsHtml5Parser::MaybePostContinueEvent()
{
  NS_PRECONDITION(mLifeCycle != TERMINATED, 
                  "Tried to post continue event when the parser is done.");
  if (mContinueEvent) {
    return; 
  }
  
  
  nsCOMPtr<nsIRunnable> event = new nsHtml5ParserContinueEvent(this);
  if (NS_FAILED(NS_DispatchToCurrentThread(event))) {
    NS_WARNING("failed to dispatch parser continuation event");
  } else {
    mContinueEvent = event;
  }
}

void
nsHtml5Parser::Suspend()
{
  mSuspending = PR_TRUE;
}

nsresult
nsHtml5Parser::Initialize(nsIDocument* aDoc,
                          nsIURI* aURI,
                          nsISupports* aContainer,
                          nsIChannel* aChannel)
{
  nsresult rv = nsContentSink::Init(aDoc, aURI, aContainer, aChannel);
  NS_ENSURE_SUCCESS(rv, rv);
  aDoc->AddObserver(this);
  return NS_OK;
}

nsresult
nsHtml5Parser::ProcessBASETag(nsIContent* aContent)
{
  NS_ASSERTION(aContent, "missing base-element");
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

void
nsHtml5Parser::UpdateStyleSheet(nsIContent* aElement)
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
nsHtml5Parser::SetScriptElement(nsIContent* aScript)
{
  mScriptElement = aScript;
}



void
nsHtml5Parser::UpdateChildCounts()
{
  
}

nsresult
nsHtml5Parser::FlushTags()
{
    return NS_OK;
}

void
nsHtml5Parser::PostEvaluateScript(nsIScriptElement *aElement)
{
  nsCOMPtr<nsIHTMLDocument> htmlDocument = do_QueryInterface(mDocument);
  NS_ASSERTION(htmlDocument, "Document didn't QI into HTML document.");
  htmlDocument->ScriptExecuted(aElement);
}


#ifdef GATHER_DOCWRITE_STATISTICS
PRUint32 nsHtml5Parser::sUnsafeDocWrites = 0;
PRUint32 nsHtml5Parser::sTokenSafeDocWrites = 0;
PRUint32 nsHtml5Parser::sTreeSafeDocWrites = 0;
#endif
