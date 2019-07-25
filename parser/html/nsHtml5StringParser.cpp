





































#include "nsHtml5StringParser.h"
#include "nsHtml5TreeOpExecutor.h"
#include "nsHtml5TreeBuilder.h"
#include "nsHtml5Tokenizer.h"
#include "nsIContent.h"
#include "nsIDocument.h"
#include "nsIDOMDocumentFragment.h"
#include "nsHtml5DependentUTF16Buffer.h"

NS_IMPL_ISUPPORTS0(nsHtml5StringParser)

nsHtml5StringParser::nsHtml5StringParser()
  : mExecutor(new nsHtml5TreeOpExecutor())
  , mTreeBuilder(new nsHtml5TreeBuilder(mExecutor, nsnull))
  , mTokenizer(new nsHtml5Tokenizer(mTreeBuilder, false))
{
  MOZ_COUNT_CTOR(nsHtml5StringParser);
  mAtomTable.Init(); 
  mTokenizer->setInterner(&mAtomTable);
}

nsHtml5StringParser::~nsHtml5StringParser()
{
  MOZ_COUNT_DTOR(nsHtml5StringParser);
}

nsresult
nsHtml5StringParser::ParseHtml5Fragment(const nsAString& aSourceBuffer,
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

  mExecutor->Init(doc, uri, nsnull, nsnull);

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
  mExecutor->Reset();
  return NS_OK;
}
