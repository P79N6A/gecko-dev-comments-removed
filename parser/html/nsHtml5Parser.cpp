







































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
#include "nsIDOMDocumentFragment.h"
#include "nsHtml5DependentUTF16Buffer.h"

NS_INTERFACE_TABLE_HEAD(nsHtml5Parser)
  NS_INTERFACE_TABLE2(nsHtml5Parser, nsIParser, nsISupportsWeakReference)
  NS_INTERFACE_TABLE_TO_MAP_SEGUE_CYCLE_COLLECTION(nsHtml5Parser)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsHtml5Parser)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsHtml5Parser)

NS_IMPL_CYCLE_COLLECTION_CLASS(nsHtml5Parser)

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(nsHtml5Parser)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR_AMBIGUOUS(mExecutor,
                                                       nsIContentSink)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR_AMBIGUOUS(mStreamParser,
                                                       nsIStreamListener)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(nsHtml5Parser)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mExecutor)
  tmp->DropStreamParser();
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

nsHtml5Parser::nsHtml5Parser()
  : mFirstBuffer(new nsHtml5OwningUTF16Buffer((void*)nsnull))
  , mLastBuffer(mFirstBuffer)
  , mExecutor(new nsHtml5TreeOpExecutor())
  , mTreeBuilder(new nsHtml5TreeBuilder(mExecutor, nsnull))
  , mTokenizer(new nsHtml5Tokenizer(mTreeBuilder, false))
  , mRootContextLineNumber(1)
{
  mAtomTable.Init(); 
  mTokenizer->setInterner(&mAtomTable);
  
}

nsHtml5Parser::~nsHtml5Parser()
{
  mTokenizer->end();
  if (mDocWriteSpeculativeTokenizer) {
    mDocWriteSpeculativeTokenizer->end();
  }
}

NS_IMETHODIMP_(void)
nsHtml5Parser::SetContentSink(nsIContentSink* aSink)
{
  NS_ASSERTION(aSink == static_cast<nsIContentSink*> (mExecutor), 
               "Attempt to set a foreign sink.");
}

NS_IMETHODIMP_(nsIContentSink*)
nsHtml5Parser::GetContentSink()
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
  NS_ASSERTION(!strcmp(aCommand, "view") || !strcmp(aCommand, "view-source"),
      "Parser command was not view");
}

NS_IMETHODIMP_(void)
nsHtml5Parser::SetCommand(eParserCommands aParserCommand)
{
  NS_ASSERTION(aParserCommand == eViewNormal, 
               "Parser command was not eViewNormal.");
}

NS_IMETHODIMP_(void)
nsHtml5Parser::SetDocumentCharset(const nsACString& aCharset,
                                  PRInt32 aCharsetSource)
{
  NS_PRECONDITION(!mExecutor->HasStarted(),
                  "Document charset set too late.");
  NS_PRECONDITION(mStreamParser, "Setting charset on a script-only parser.");
  nsCAutoString trimmed;
  trimmed.Assign(aCharset);
  trimmed.Trim(" \t\r\n\f");
  mStreamParser->SetDocumentCharset(trimmed, aCharsetSource);
  mExecutor->SetDocumentCharsetAndSource(trimmed,
                                         aCharsetSource);
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
  NS_NOTREACHED("Don't call. For interface compat only.");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP_(void)
nsHtml5Parser::BlockParser()
{
  mBlocked = true;
}

NS_IMETHODIMP_(void)
nsHtml5Parser::UnblockParser()
{
  mBlocked = false;
}

NS_IMETHODIMP_(bool)
nsHtml5Parser::IsParserEnabled()
{
  return !mBlocked;
}

NS_IMETHODIMP_(bool)
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
                  "Tried to start parse without initializing the parser.");
  NS_PRECONDITION(mStreamParser, 
                  "Can't call this Parse() variant on script-created parser");
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
                     bool aLastCall,
                     nsDTDMode aMode) 
{
  NS_PRECONDITION(!mExecutor->IsFragmentMode(),
                  "Document.write called in fragment mode!");
  if (mExecutor->IsBroken()) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  if (aSourceBuffer.Length() > PR_INT32_MAX) {
    mExecutor->MarkAsBroken();
    return NS_ERROR_OUT_OF_MEMORY;
  }

  
  
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
    mDocumentClosed = true;
    if (!mBlocked && !mInDocumentWrite) {
      ParseUntilBlocked();
    }
    return NS_OK;
  }

  
  

  NS_ASSERTION(IsInsertionPointDefined(),
               "Doc.write reached parser with undefined insertion point.");

  NS_ASSERTION(!(mStreamParser && !aKey),
               "Got a null key in a non-script-created parser");

  
  if (aSourceBuffer.IsEmpty()) {
    return NS_OK;
  }

  
  
  
  mozilla::AutoRestore<bool> guard(mInDocumentWrite);
  mInDocumentWrite = true;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  
  
  
  
  
  
  
  
  nsRefPtr<nsHtml5OwningUTF16Buffer> prevSearchBuf;
  nsRefPtr<nsHtml5OwningUTF16Buffer> firstLevelMarker;

  if (aKey) {
    if (mFirstBuffer == mLastBuffer) {
      nsHtml5OwningUTF16Buffer* keyHolder = new nsHtml5OwningUTF16Buffer(aKey);
      keyHolder->next = mLastBuffer;
      mFirstBuffer = keyHolder;
    } else if (mFirstBuffer->key != aKey) {
      prevSearchBuf = mFirstBuffer;
      for (;;) {
        if (prevSearchBuf->next == mLastBuffer) {
          
          nsHtml5OwningUTF16Buffer* keyHolder =
            new nsHtml5OwningUTF16Buffer(aKey);
          keyHolder->next = mFirstBuffer;
          mFirstBuffer = keyHolder;
          prevSearchBuf = nsnull;
          break;
        }
        if (prevSearchBuf->next->key == aKey) {
          
          break;
        }
        prevSearchBuf = prevSearchBuf->next;
      }
    } 

    
    
  } else {
    
    
    
    
    firstLevelMarker = new nsHtml5OwningUTF16Buffer((void*)nsnull);
    if (mFirstBuffer == mLastBuffer) {
      firstLevelMarker->next = mLastBuffer;
      mFirstBuffer = firstLevelMarker;
    } else {
      prevSearchBuf = mFirstBuffer;
      while (prevSearchBuf->next != mLastBuffer) {
        prevSearchBuf = prevSearchBuf->next;
      }
      firstLevelMarker->next = mLastBuffer;
      prevSearchBuf->next = firstLevelMarker;
    }
  }

  nsHtml5DependentUTF16Buffer stackBuffer(aSourceBuffer);

  while (!mBlocked && stackBuffer.hasMore()) {
    stackBuffer.adjust(mLastWasCR);
    mLastWasCR = false;
    if (stackBuffer.hasMore()) {
      PRInt32 lineNumberSave;
      bool inRootContext = (!mStreamParser && (aKey == mRootContextKey));
      if (inRootContext) {
        mTokenizer->setLineNumber(mRootContextLineNumber);
      } else {
        
        
        lineNumberSave = mTokenizer->getLineNumber();
      }

      mLastWasCR = mTokenizer->tokenizeBuffer(&stackBuffer);

      if (inRootContext) {
        mRootContextLineNumber = mTokenizer->getLineNumber();
      } else {
        mTokenizer->setLineNumber(lineNumberSave);
      }

      if (mTreeBuilder->HasScript()) {
        mTreeBuilder->Flush(); 
        mExecutor->FlushDocumentWrite(); 
        
        
        if (mExecutor->IsComplete()) {
          return NS_OK;
        }
      }
      
    }
  }

  nsRefPtr<nsHtml5OwningUTF16Buffer> heapBuffer;
  if (stackBuffer.hasMore()) {
    
    
    heapBuffer = stackBuffer.FalliblyCopyAsOwningBuffer();
    if (!heapBuffer) {
      
      mExecutor->MarkAsBroken();
      return NS_ERROR_OUT_OF_MEMORY;
    }
  }

  if (heapBuffer) {
    
    
    
    if (aKey) {
      NS_ASSERTION(mFirstBuffer != mLastBuffer,
        "Where's the keyholder?");
      
      
      if (mFirstBuffer->key == aKey) {
        NS_ASSERTION(!prevSearchBuf,
          "Non-null prevSearchBuf when mFirstBuffer is the key holder?");
        heapBuffer->next = mFirstBuffer;
        mFirstBuffer = heapBuffer;
      } else {
        if (!prevSearchBuf) {
          prevSearchBuf = mFirstBuffer;
        }
        
        
        while (prevSearchBuf->next->key != aKey) {
          prevSearchBuf = prevSearchBuf->next;
        }
        heapBuffer->next = prevSearchBuf->next;
        prevSearchBuf->next = heapBuffer;
      }
    } else {
      NS_ASSERTION(firstLevelMarker, "How come we don't have a marker.");
      firstLevelMarker->Swap(heapBuffer);
    }
  }

  if (!mBlocked) { 
    NS_ASSERTION(!stackBuffer.hasMore(),
      "Buffer wasn't tokenized to completion?");
    
    mTreeBuilder->Flush(); 
    mExecutor->FlushDocumentWrite(); 
  } else if (stackBuffer.hasMore()) {
    
    
    
    if (!mDocWriteSpeculatorActive) {
      mDocWriteSpeculatorActive = true;
      if (!mDocWriteSpeculativeTreeBuilder) {
        
        mDocWriteSpeculativeTreeBuilder =
            new nsHtml5TreeBuilder(nsnull, mExecutor->GetStage());
        mDocWriteSpeculativeTreeBuilder->setScriptingEnabled(
            mTreeBuilder->isScriptingEnabled());
        mDocWriteSpeculativeTokenizer =
            new nsHtml5Tokenizer(mDocWriteSpeculativeTreeBuilder, false);
        mDocWriteSpeculativeTokenizer->setInterner(&mAtomTable);
        mDocWriteSpeculativeTokenizer->start();
      }
      mDocWriteSpeculativeTokenizer->resetToDataState();
      mDocWriteSpeculativeTreeBuilder->loadState(mTreeBuilder, &mAtomTable);
      mDocWriteSpeculativeLastWasCR = false;
    }

    
    
    
    
    

    
    
    
    while (stackBuffer.hasMore()) {
      stackBuffer.adjust(mDocWriteSpeculativeLastWasCR);
      if (stackBuffer.hasMore()) {
        mDocWriteSpeculativeLastWasCR =
            mDocWriteSpeculativeTokenizer->tokenizeBuffer(&stackBuffer);
      }
    }

    mDocWriteSpeculativeTreeBuilder->Flush();
    mDocWriteSpeculativeTreeBuilder->DropHandles();
    mExecutor->FlushSpeculativeLoads();
  }

  return NS_OK;
}




NS_IMETHODIMP_(void *)
nsHtml5Parser::GetRootContextKey()
{
  return mRootContextKey;
}

NS_IMETHODIMP
nsHtml5Parser::Terminate()
{
  
  
  if (mExecutor->IsComplete()) {
    return NS_OK;
  }
  
  
  nsCOMPtr<nsIParser> kungFuDeathGrip(this);
  nsRefPtr<nsHtml5StreamParser> streamKungFuDeathGrip(mStreamParser);
  nsRefPtr<nsHtml5TreeOpExecutor> treeOpKungFuDeathGrip(mExecutor);
  if (mStreamParser) {
    mStreamParser->Terminate();
  }
  return mExecutor->DidBuildModel(true);
}

NS_IMETHODIMP
nsHtml5Parser::ParseFragment(const nsAString& aSourceBuffer,
                             nsTArray<nsString>& aTagStack)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

nsresult
nsHtml5Parser::ParseHtml5Fragment(const nsAString& aSourceBuffer,
                                  nsIContent* aTargetNode,
                                  nsIAtom* aContextLocalName,
                                  PRInt32 aContextNamespace,
                                  bool aQuirks,
                                  bool aPreventScriptExecution)
{
  NS_ENSURE_TRUE(aSourceBuffer.Length() <= PR_INT32_MAX,
      NS_ERROR_OUT_OF_MEMORY);
  nsIDocument* doc = aTargetNode->OwnerDoc();
  
  nsIURI* uri = doc->GetDocumentURI();
  NS_ENSURE_TRUE(uri, NS_ERROR_NOT_AVAILABLE);

  mExecutor->EnableFragmentMode(aPreventScriptExecution);

  Initialize(doc, uri, nsnull, nsnull);

  mExecutor->SetParser(this);
  mExecutor->SetNodeInfoManager(doc->NodeInfoManager());

  nsIContent* target = aTargetNode;
  mTreeBuilder->setFragmentContext(aContextLocalName,
                                   aContextNamespace,
                                   &target,
                                   aQuirks);

#ifdef DEBUG
  if (!aPreventScriptExecution) {
    NS_ASSERTION(!aTargetNode->IsInDoc(),
        "If script execution isn't prevented, "
        "the target node must not be in doc.");
    nsCOMPtr<nsIDOMDocumentFragment> domFrag = do_QueryInterface(aTargetNode);
    NS_ASSERTION(domFrag,
        "If script execution isn't prevented, must parse to DOM fragment.");
  }
#endif

  NS_PRECONDITION(!mExecutor->HasStarted(),
                  "Tried to start parse without initializing the parser.");
  mTreeBuilder->setScriptingEnabled(mExecutor->IsScriptEnabled());
  mTokenizer->start();
  mExecutor->Start(); 
  if (!aSourceBuffer.IsEmpty()) {
    bool lastWasCR = false;
    nsHtml5DependentUTF16Buffer buffer(aSourceBuffer);    
    while (buffer.hasMore()) {
      buffer.adjust(lastWasCR);
      lastWasCR = false;
      if (buffer.hasMore()) {
        lastWasCR = mTokenizer->tokenizeBuffer(&buffer);
        if (mTreeBuilder->HasScript()) {
          
          
          mTreeBuilder->Flush(); 
          mExecutor->FlushDocumentWrite(); 
        }
      }
    }
  }
  mTokenizer->eof();
  mTreeBuilder->StreamEnded();
  mTreeBuilder->Flush();
  mExecutor->FlushDocumentWrite();
  mTokenizer->end();
  mExecutor->DropParserAndPerfHint();
  mExecutor->DropHeldElements();
  mTreeBuilder->DropHandles();
  mAtomTable.Clear();
  return NS_OK;
}

NS_IMETHODIMP
nsHtml5Parser::BuildModel()
{
  NS_NOTREACHED("Don't call this!");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsHtml5Parser::CancelParsingEvents()
{
  NS_NOTREACHED("Don't call this!");
  return NS_ERROR_NOT_IMPLEMENTED;
}

void
nsHtml5Parser::Reset()
{
  NS_PRECONDITION(mExecutor->IsFragmentMode(),
                  "Reset called on a non-fragment parser.");
  mExecutor->Reset();
  mLastWasCR = false;
  UnblockParser();
  mDocumentClosed = false;
  mStreamParser = nsnull;
  mRootContextLineNumber = 1;
  mParserInsertedScriptsBeingEvaluated = 0;
  mRootContextKey = nsnull;
  mAtomTable.Clear(); 
  
  mFirstBuffer->next = nsnull;
  mFirstBuffer->setStart(0);
  mFirstBuffer->setEnd(0);
  mLastBuffer = mFirstBuffer;
}

bool
nsHtml5Parser::CanInterrupt()
{
  
  
  return true;
}

bool
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
nsHtml5Parser::MarkAsNotScriptCreated(const char* aCommand)
{
  NS_PRECONDITION(!mStreamParser, "Must not call this twice.");
  eParserMode mode = NORMAL;
  if (!nsCRT::strcmp(aCommand, "view-source")) {
    mode = VIEW_SOURCE_HTML;
    
  }
#ifdef DEBUG
  else {
    NS_ASSERTION(!nsCRT::strcmp(aCommand, "view"),
        "Unsupported parser command!");
  }
#endif
  mStreamParser = new nsHtml5StreamParser(mExecutor, this, mode);
}

bool
nsHtml5Parser::IsScriptCreated()
{
  return !mStreamParser;
}




void
nsHtml5Parser::ParseUntilBlocked()
{
  NS_PRECONDITION(!mExecutor->IsFragmentMode(),
                  "ParseUntilBlocked called in fragment mode.");

  if (mBlocked || mExecutor->IsComplete() || mExecutor->IsBroken()) {
    return;
  }
  NS_ASSERTION(mExecutor->HasStarted(), "Bad life cycle.");
  NS_ASSERTION(!mInDocumentWrite,
    "ParseUntilBlocked entered while in doc.write!");

  mDocWriteSpeculatorActive = false;

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
          mExecutor->FlushDocumentWrite();
          mTokenizer->end();
          return;            
        }
        
        NS_ASSERTION(!mLastBuffer->getStart() && !mLastBuffer->getEnd(),
                     "Sentinel buffer had its indeces changed.");
        if (mStreamParser) {
          if (mReturnToStreamParserPermitted &&
              !mExecutor->IsScriptExecuting()) {
            mTreeBuilder->Flush();
            mReturnToStreamParserPermitted = false;
            mStreamParser->ContinueAfterScripts(mTokenizer,
                                                mTreeBuilder,
                                                mLastWasCR);
          }
        } else {
          
          mTreeBuilder->Flush();
          
          
          NS_ASSERTION(mExecutor->IsInFlushLoop(),
              "How did we come here without being in the flush loop?");
        }
        return; 
      }
      mFirstBuffer = mFirstBuffer->next;
      continue;
    }

    if (mBlocked || mExecutor->IsComplete()) {
      return;
    }

    
    mFirstBuffer->adjust(mLastWasCR);
    mLastWasCR = false;
    if (mFirstBuffer->hasMore()) {
      bool inRootContext = (!mStreamParser &&
                              (mFirstBuffer->key == mRootContextKey));
      if (inRootContext) {
        mTokenizer->setLineNumber(mRootContextLineNumber);
      }
      mLastWasCR = mTokenizer->tokenizeBuffer(mFirstBuffer);
      if (inRootContext) {
        mRootContextLineNumber = mTokenizer->getLineNumber();
      }
      if (mTreeBuilder->HasScript()) {
        mTreeBuilder->Flush();
        mExecutor->FlushDocumentWrite();
      }
      if (mBlocked) {
        return;
      }
    }
    continue;
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

void
nsHtml5Parser::StartTokenizer(bool aScriptingEnabled) {
  mTreeBuilder->setScriptingEnabled(aScriptingEnabled);
  mTokenizer->start();
}

void
nsHtml5Parser::InitializeDocWriteParserState(nsAHtml5TreeBuilderState* aState,
                                             PRInt32 aLine)
{
  mTokenizer->resetToDataState();
  mTokenizer->setLineNumber(aLine);
  mTreeBuilder->loadState(aState, &mAtomTable);
  mLastWasCR = false;
  mReturnToStreamParserPermitted = true;
}

void
nsHtml5Parser::ContinueAfterFailedCharsetSwitch()
{
  NS_PRECONDITION(mStreamParser, 
    "Tried to continue after failed charset switch without a stream parser");
  mStreamParser->ContinueAfterFailedCharsetSwitch();
}
