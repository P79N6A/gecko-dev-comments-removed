







































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
  NS_INTERFACE_TABLE1(nsHtml5Parser, nsIParser)
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
  , mAtomTable(new nsHtml5AtomTable())
{
  mExecutor->SetTreeBuilder(mTreeBuilder);
  mAtomTable->Init(); 
  mTokenizer->setInterner(mAtomTable);
  
}

nsHtml5Parser::~nsHtml5Parser()
{
  while (mFirstBuffer) {
     nsHtml5UTF16Buffer* old = mFirstBuffer;
     mFirstBuffer = mFirstBuffer->next;
     delete old;
  }
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
  NS_PRECONDITION(mExecutor->GetLifeCycle() == NOT_STARTED,
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
  if (!mStreamParser) {
    mStreamParser = new nsHtml5StreamParser(mTokenizer, mExecutor, this);
  }
  NS_ADDREF(*aListener = mStreamParser);
  return NS_OK;
}

NS_IMETHODIMP
nsHtml5Parser::ContinueParsing()
{
  UnblockParser();
  return ContinueInterruptedParsing();
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
  
  mExecutor->MaybeFlush();
  ParseUntilSuspend();
  return NS_OK;
}

NS_IMETHODIMP_(void)
nsHtml5Parser::BlockParser()
{
  mBlocked = PR_TRUE;
  if (mStreamParser) {
    mStreamParser->Block();
  }
}

NS_IMETHODIMP_(void)
nsHtml5Parser::UnblockParser()
{
  mBlocked = PR_FALSE;
  if (mStreamParser) {
    mStreamParser->Unblock();
  }
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
  



  NS_PRECONDITION(mExecutor->GetLifeCycle() == NOT_STARTED, 
                  "Tried to start parse without initializing the parser properly.");
  if (!mStreamParser) {
    mStreamParser = new nsHtml5StreamParser(mTokenizer, mExecutor, this);
  }
  mStreamParser->SetObserver(aObserver);
  mTokenizer->setEncodingDeclarationHandler(mStreamParser);
  mExecutor->SetStreamParser(mStreamParser);
  mExecutor->SetParser(this);
  mTreeBuilder->setScriptingEnabled(mExecutor->IsScriptEnabled());
  mExecutor->AllowInterrupts();
  mRootContextKey = aKey;
  mExecutor->SetParser(this);
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

  
  switch (mExecutor->GetLifeCycle()) {
    case TERMINATED:
      return NS_OK;
    case NOT_STARTED:
      mExecutor->SetParser(this);
      mTreeBuilder->setScriptingEnabled(mExecutor->IsScriptEnabled());
      mTokenizer->start();
      mExecutor->SetLifeCycle(PARSING);
      break;
    default:
      break;
  }

  if (aLastCall && aSourceBuffer.IsEmpty() && aKey == GetRootContextKey()) {
    
    mExecutor->SetLifeCycle(STREAM_ENDING);
    MaybePostContinueEvent();
    return NS_OK;
  }

  

  PRInt32 lineNumberSave = mTokenizer->getLineNumber();

  if (!aSourceBuffer.IsEmpty()) {
    nsHtml5UTF16Buffer* buffer = new nsHtml5UTF16Buffer(aSourceBuffer.Length());
    memcpy(buffer->getBuffer(), aSourceBuffer.BeginReading(), aSourceBuffer.Length() * sizeof(PRUnichar));
    buffer->setEnd(aSourceBuffer.Length());
    if (!mBlocked) {
      mExecutor->WillResume();
      while (buffer->hasMore()) {
        buffer->adjust(mLastWasCR);
        mLastWasCR = PR_FALSE;
        if (buffer->hasMore()) {
          mLastWasCR = mTokenizer->tokenizeBuffer(buffer);
          mExecutor->MaybeExecuteScript();
          if (mBlocked) {
            
            mExecutor->WillInterrupt();
            break;
          }
          
        }
      }
    }

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

  
  mExecutor->Flush();
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
  
  
  if (mExecutor->GetLifeCycle() == TERMINATED) {
    return NS_OK;
  }
  
  
  nsCOMPtr<nsIParser> kungFuDeathGrip(this);
  nsRefPtr<nsHtml5StreamParser> streamKungFuDeathGrip(mStreamParser);
  nsRefPtr<nsHtml5TreeOpExecutor> treeOpKungFuDeathGrip(mExecutor);
  
  
  CancelParsingEvents();
  
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
  mExecutor->ProhibitInterrupts();
  mExecutor->SetParser(this);
  mExecutor->SetNodeInfoManager(target->GetOwnerDoc()->NodeInfoManager());

  nsIContent* weakTarget = target;
  mTreeBuilder->setFragmentContext(aContextLocalName, aContextNamespace, &weakTarget, aQuirks);
  mFragmentMode = PR_TRUE;
  
  NS_PRECONDITION(mExecutor->GetLifeCycle() == NOT_STARTED, "Tried to start parse without initializing the parser properly.");
  mTreeBuilder->setScriptingEnabled(mExecutor->IsScriptEnabled());
  mTokenizer->start();
  mExecutor->SetLifeCycle(PARSING);
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
  mExecutor->SetLifeCycle(TERMINATED);
  mTokenizer->eof();
  mExecutor->Flush();
  mTokenizer->end();
  mExecutor->DropParserAndPerfHint();
  mAtomTable->Clear();
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
  mExecutor->Reset();
  mLastWasCR = PR_FALSE;
  mFragmentMode = PR_FALSE;
  UnblockParser();
  mSuspending = PR_FALSE;
  mStreamParser = nsnull;
  mRootContextKey = nsnull;
  mContinueEvent = nsnull;  
  mAtomTable->Clear(); 
  
  while (mFirstBuffer->next) {
    nsHtml5UTF16Buffer* oldBuf = mFirstBuffer;
    mFirstBuffer = mFirstBuffer->next;
    delete oldBuf;
  }
  mFirstBuffer->setStart(0);
  mFirstBuffer->setEnd(0);
}

PRBool
nsHtml5Parser::CanInterrupt()
{
  return !mFragmentMode;
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
nsHtml5Parser::ParseUntilSuspend()
{
  NS_PRECONDITION(!mFragmentMode, "ParseUntilSuspend called in fragment mode.");
  NS_PRECONDITION(!mExecutor->NeedsCharsetSwitch(), "ParseUntilSuspend called when charset switch needed.");

  if (mBlocked) {
    return;
  }

  switch (mExecutor->GetLifeCycle()) {
    case TERMINATED:
      return;
    case NOT_STARTED:
      NS_NOTREACHED("Bad life cycle!");
      break;
    default:
      break;
  }

  mExecutor->WillResume();
  mSuspending = PR_FALSE;
  for (;;) {
    if (!mFirstBuffer->hasMore()) {
      if (mFirstBuffer == mLastBuffer) {
        switch (mExecutor->GetLifeCycle()) {
          case TERMINATED:
            
            return;
          case PARSING:
            
            mFirstBuffer->setStart(0);
            mFirstBuffer->setEnd(0);
            if (mStreamParser) {
              mStreamParser->ParseUntilSuspend();
            }
            return; 
          case STREAM_ENDING:
            if (mStreamParser && !mStreamParser->IsDone()) { 
              mStreamParser->ParseUntilSuspend();            
            } else {
              
              mExecutor->DidBuildModel(PR_FALSE);
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

    if (mBlocked || (mExecutor->GetLifeCycle() == TERMINATED)) {
      return;
    }

    
    mFirstBuffer->adjust(mLastWasCR);
    mLastWasCR = PR_FALSE;
    if (mFirstBuffer->hasMore()) {
      mLastWasCR = mTokenizer->tokenizeBuffer(mFirstBuffer);
      NS_ASSERTION(!(mExecutor->HasScriptElement() && mExecutor->NeedsCharsetSwitch()), "Can't have both script and charset switch.");
      mExecutor->IgnoreCharsetSwitch();
      mExecutor->MaybeExecuteScript();
      if (mBlocked) {
        mExecutor->WillInterrupt();
        return;
      }
      if (mSuspending) {
        MaybePostContinueEvent();
        mExecutor->WillInterrupt();
        return;
      }
    }
    continue;
  }
}

void
nsHtml5Parser::MaybePostContinueEvent()
{
  NS_PRECONDITION(mExecutor->GetLifeCycle() != TERMINATED, 
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
  if (mStreamParser) {
    mStreamParser->Suspend();
  }
}

nsresult
nsHtml5Parser::Initialize(nsIDocument* aDoc,
                          nsIURI* aURI,
                          nsISupports* aContainer,
                          nsIChannel* aChannel)
{
  return mExecutor->Init(aDoc, aURI, aContainer, aChannel);
}

