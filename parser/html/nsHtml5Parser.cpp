







































#include "nsCompatibility.h"
#include "nsScriptLoader.h"
#include "nsNetUtil.h"
#include "nsIStyleSheetLinkingElement.h"
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
#include "nsHtml5AtomTable.h"








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



NS_INTERFACE_TABLE_HEAD(nsHtml5Parser)
  NS_INTERFACE_TABLE2(nsHtml5Parser, nsIParser, nsISupportsWeakReference)
  NS_INTERFACE_TABLE_TO_MAP_SEGUE_CYCLE_COLLECTION(nsHtml5Parser)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsHtml5Parser)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsHtml5Parser)

NS_IMPL_CYCLE_COLLECTION_CLASS(nsHtml5Parser)

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(nsHtml5Parser)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR_AMBIGUOUS(mExecutor, nsIContentSink)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR_AMBIGUOUS(mStreamParser, nsIStreamListener)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(nsHtml5Parser)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mExecutor)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mStreamParser)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

nsHtml5Parser::nsHtml5Parser()
  : mFirstBuffer(new nsHtml5UTF16Buffer(0))
  , mLastBuffer(mFirstBuffer)
  , mExecutor(new nsHtml5TreeOpExecutor())
  , mTreeBuilder(new nsHtml5TreeBuilder(mExecutor))
  , mTokenizer(new nsHtml5Tokenizer(mTreeBuilder))
{
  mAtomTable.Init(); 
  mTokenizer->setInterner(&mAtomTable);
  
}

nsHtml5Parser::~nsHtml5Parser()
{
  mTokenizer->end();
  mFirstBuffer = nsnull;
}

NS_IMETHODIMP_(void)
nsHtml5Parser::SetContentSink(nsIContentSink* aSink)
{
  NS_ASSERTION(aSink == static_cast<nsIContentSink*> (mExecutor), 
               "Attempt to set a foreign sink.");
}

NS_IMETHODIMP_(nsIContentSink*)
nsHtml5Parser::GetContentSink(void)
{
  return static_cast<nsIContentSink*> (mExecutor);
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
  NS_PRECONDITION(!mExecutor->HasStarted(),
                  "Document charset set too late.");
  NS_PRECONDITION(mStreamParser, "Tried to set charset on a script-only parser.");
  mStreamParser->SetDocumentCharset(aCharset, aCharsetSource);
  mExecutor->SetDocumentCharset((nsACString&)aCharset);
}

NS_IMETHODIMP_(void)
nsHtml5Parser::SetParserFilter(nsIParserFilter* aFilter)
{
  NS_ERROR("Attempt to set a parser filter on HTML5 parser.");
}

NS_IMETHODIMP
nsHtml5Parser::GetChannel(nsIChannel** aChannel)
{
  if (mStreamParser) {
    return mStreamParser->GetChannel(aChannel);
  } else {
    return NS_ERROR_NOT_AVAILABLE;
  }
}

NS_IMETHODIMP
nsHtml5Parser::GetDTD(nsIDTD** aDTD)
{
  *aDTD = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
nsHtml5Parser::GetStreamListener(nsIStreamListener** aListener)
{
  NS_IF_ADDREF(*aListener = mStreamParser);
  return NS_OK;
}

NS_IMETHODIMP
nsHtml5Parser::ContinueInterruptedParsing()
{
  
  
  
  if (mExecutor->IsScriptExecuting()) {
    return NS_OK;
  }
  
  
  
  
  nsCOMPtr<nsIParser> kungFuDeathGrip(this);
  nsRefPtr<nsHtml5StreamParser> streamKungFuDeathGrip(mStreamParser);
  nsRefPtr<nsHtml5TreeOpExecutor> treeOpKungFuDeathGrip(mExecutor);
  CancelParsingEvents(); 
  ParseUntilScript();
  return NS_OK;
}

NS_IMETHODIMP_(void)
nsHtml5Parser::BlockParser()
{
  mBlocked = PR_TRUE;
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
  return mExecutor->IsComplete();
}

NS_IMETHODIMP
nsHtml5Parser::Parse(nsIURI* aURL, 
                     nsIRequestObserver* aObserver,
                     void* aKey,
                     nsDTDMode aMode) 
{
  



  NS_PRECONDITION(!mExecutor->HasStarted(), 
                  "Tried to start parse without initializing the parser properly.");
  NS_PRECONDITION(mStreamParser, 
                  "Can't call this variant of Parse() on script-created parser");
  mStreamParser->SetObserver(aObserver);
  mExecutor->SetStreamParser(mStreamParser);
  mExecutor->SetParser(this);
  mRootContextKey = aKey;
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

  
  
  nsCOMPtr<nsIParser> kungFuDeathGrip(this);
  
  
  
  nsRefPtr<nsHtml5StreamParser> streamKungFuDeathGrip(mStreamParser);
  nsRefPtr<nsHtml5TreeOpExecutor> treeOpKungFuDeathGrip(mExecutor);

  
  if (!mExecutor->HasStarted()) {
    NS_ASSERTION(!mStreamParser,
                 "Had stream parser but document.write started life cycle.");
    mExecutor->SetParser(this);
    mTreeBuilder->setScriptingEnabled(mExecutor->IsScriptEnabled());
    mTokenizer->start();
    mExecutor->Start();
    




    mExecutor->WillBuildModel(eDTDMode_unknown);
  }
  if (mExecutor->IsComplete()) {
    return NS_OK;
  }
  if (aLastCall && aSourceBuffer.IsEmpty() && aKey == GetRootContextKey()) {
    
      NS_ASSERTION(!mStreamParser,
                   "Had stream parser but got document.close().");
    mDocumentClosed = PR_TRUE;
    
    MaybePostContinueEvent();
    return NS_OK;
  }

  NS_PRECONDITION(IsInsertionPointDefined(), 
                  "Document.write called when insertion point not defined.");

  if (aSourceBuffer.IsEmpty()) {
    return NS_OK;
  }

  PRInt32 lineNumberSave = mTokenizer->getLineNumber();

  nsRefPtr<nsHtml5UTF16Buffer> buffer = new nsHtml5UTF16Buffer(aSourceBuffer.Length());
  memcpy(buffer->getBuffer(), aSourceBuffer.BeginReading(), aSourceBuffer.Length() * sizeof(PRUnichar));
  buffer->setEnd(aSourceBuffer.Length());

  
  
  
  
  
  
  
  
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

  if (!mBlocked) {
    
    while (buffer->hasMore()) {
      buffer->adjust(mLastWasCR);
      mLastWasCR = PR_FALSE;
      if (buffer->hasMore()) {
        mLastWasCR = mTokenizer->tokenizeBuffer(buffer);
        if (mTreeBuilder->HasScript()) {
          
          mTreeBuilder->Flush(); 
          mExecutor->Flush(); 
        }
        if (mBlocked) {
          
          break;
        }
        
      }
    }
  }

  if (!mBlocked) { 
    
    mTreeBuilder->flushCharacters(); 
    mTreeBuilder->Flush(); 
    mExecutor->Flush(); 
  } else if (!mStreamParser && buffer->hasMore() && aKey == mRootContextKey) {
    
    
    
    MaybePostContinueEvent();
  }

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
  
  
  if (mExecutor->IsComplete()) {
    return NS_OK;
  }
  
  
  nsCOMPtr<nsIParser> kungFuDeathGrip(this);
  nsRefPtr<nsHtml5StreamParser> streamKungFuDeathGrip(mStreamParser);
  nsRefPtr<nsHtml5TreeOpExecutor> treeOpKungFuDeathGrip(mExecutor);
  
  
  CancelParsingEvents();
  if (mStreamParser) {
    mStreamParser->Terminate();
  }
  return mExecutor->DidBuildModel(PR_TRUE);
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

  nsCOMPtr<nsISupports> container = doc->GetContainer();
  NS_ENSURE_TRUE(container, NS_ERROR_NOT_AVAILABLE);

  Initialize(doc, uri, container, nsnull);

  
  mExecutor->SetBaseUriFromDocument();
  mExecutor->SetParser(this);
  mExecutor->SetNodeInfoManager(target->GetOwnerDoc()->NodeInfoManager());

  nsIContent* weakTarget = target;
  mTreeBuilder->setFragmentContext(aContextLocalName, aContextNamespace, &weakTarget, aQuirks);
  mFragmentMode = PR_TRUE;
  
  NS_PRECONDITION(!mExecutor->HasStarted(), "Tried to start parse without initializing the parser properly.");
  mTreeBuilder->setScriptingEnabled(mExecutor->IsScriptEnabled());
  mTokenizer->start();
  mExecutor->Start(); 
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
        mExecutor->MaybePreventExecution();
      }
    }
  }
  mTokenizer->eof();
  mTreeBuilder->StreamEnded();
  mTreeBuilder->Flush();
  mExecutor->Flush();
  mTokenizer->end();
  mExecutor->DropParserAndPerfHint();
  mAtomTable.Clear();
  return NS_OK;
}

NS_IMETHODIMP
nsHtml5Parser::BuildModel(void)
{
  NS_NOTREACHED("Don't call this!");
  return NS_ERROR_NOT_IMPLEMENTED;
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
  mExecutor->Reset();
  mLastWasCR = PR_FALSE;
  mFragmentMode = PR_FALSE;
  UnblockParser();
  mDocumentClosed = PR_FALSE;
  mStreamParser = nsnull;
  mParserInsertedScriptsBeingEvaluated = 0;
  mRootContextKey = nsnull;
  mContinueEvent = nsnull;  
  mAtomTable.Clear(); 
  
  mFirstBuffer->next = nsnull;
  mFirstBuffer->setStart(0);
  mFirstBuffer->setEnd(0);
}

PRBool
nsHtml5Parser::CanInterrupt()
{
  return !mFragmentMode;
}

PRBool
nsHtml5Parser::IsInsertionPointDefined()
{
  return !mExecutor->IsFlushing() &&
    (!mStreamParser || mParserInsertedScriptsBeingEvaluated);
}

void
nsHtml5Parser::BeginEvaluatingParserInsertedScript()
{
  ++mParserInsertedScriptsBeingEvaluated;
}

void
nsHtml5Parser::EndEvaluatingParserInsertedScript()
{
  --mParserInsertedScriptsBeingEvaluated;
}

void
nsHtml5Parser::MarkAsNotScriptCreated()
{
  NS_PRECONDITION(!mStreamParser, "Must not call this twice.");
  mStreamParser = new nsHtml5StreamParser(mExecutor, this);
}

PRBool
nsHtml5Parser::IsScriptCreated()
{
  return !mStreamParser;
}




void
nsHtml5Parser::HandleParserContinueEvent(nsHtml5ParserContinueEvent* ev)
{
  
  if (mContinueEvent != ev)
    return;
  mContinueEvent = nsnull;
  NS_ASSERTION(!mExecutor->IsScriptExecuting(), "Interrupted in the middle of a script?");
  ContinueInterruptedParsing();
}

void
nsHtml5Parser::ParseUntilScript()
{
  NS_PRECONDITION(!mFragmentMode, "ParseUntilScript called in fragment mode.");

  if (mBlocked) {
    return;
  }

  if (mExecutor->IsComplete()) {
    return;
  }
  NS_ASSERTION(mExecutor->HasStarted(), "Bad life cycle.");

  mExecutor->WillResume();
  for (;;) {
    if (!mFirstBuffer->hasMore()) {
      if (mFirstBuffer == mLastBuffer) {
        if (mExecutor->IsComplete()) {
          
          return;
        }
        if (mDocumentClosed) {
          NS_ASSERTION(!mStreamParser,
                       "This should only happen with script-created parser.");
          mTokenizer->eof();
          mTreeBuilder->StreamEnded();
          mTreeBuilder->Flush();
          mExecutor->Flush();
          mTokenizer->end();
          return;            
        } else {
          
          NS_ASSERTION(!mLastBuffer->getStart(), 
            "Sentinel buffer had its indeces changed.");
          NS_ASSERTION(!mLastBuffer->getEnd(), 
            "Sentinel buffer had its indeces changed.");
          if (mStreamParser && 
              mReturnToStreamParserPermitted && 
              !mExecutor->IsScriptExecuting()) {
            mReturnToStreamParserPermitted = PR_FALSE;
            mStreamParser->ContinueAfterScripts(mTokenizer, 
                                                mTreeBuilder, 
                                                mLastWasCR);
          }
          return; 
        }
      } else {
        mFirstBuffer = mFirstBuffer->next;
        continue;
      }
    }

    if (mBlocked || mExecutor->IsComplete()) {
      return;
    }

    
    mFirstBuffer->adjust(mLastWasCR);
    mLastWasCR = PR_FALSE;
    if (mFirstBuffer->hasMore()) {
      mLastWasCR = mTokenizer->tokenizeBuffer(mFirstBuffer);
      if (mTreeBuilder->HasScript()) {
        mTreeBuilder->Flush();
        mExecutor->Flush();   
      }
      if (mBlocked) {
        
        return;
      }
    }
    continue;
  }
}

void
nsHtml5Parser::MaybePostContinueEvent()
{
  NS_PRECONDITION(!mExecutor->IsComplete(), 
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

nsresult
nsHtml5Parser::Initialize(nsIDocument* aDoc,
                          nsIURI* aURI,
                          nsISupports* aContainer,
                          nsIChannel* aChannel)
{
  if (mStreamParser && aDoc) {
    mStreamParser->SetSpeculativeLoaderWithDocument(aDoc);
  }
  return mExecutor->Init(aDoc, aURI, aContainer, aChannel);
}

void
nsHtml5Parser::StartTokenizer(PRBool aScriptingEnabled) {
  mTreeBuilder->setScriptingEnabled(aScriptingEnabled);
  mTokenizer->start();
}

void
nsHtml5Parser::InitializeDocWriteParserState(nsAHtml5TreeBuilderState* aState)
{
  mTokenizer->resetToDataState();
  mTreeBuilder->loadState(aState, &mAtomTable);
  mLastWasCR = PR_FALSE;
  mReturnToStreamParserPermitted = PR_TRUE;
}
