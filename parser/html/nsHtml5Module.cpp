




































#include "nsContentUtils.h"
#include "nsHtml5AttributeName.h"
#include "nsHtml5ElementName.h"
#include "nsHtml5HtmlAttributes.h"
#include "nsHtml5NamedCharacters.h"
#include "nsHtml5Portability.h"
#include "nsHtml5StackNode.h"
#include "nsHtml5Tokenizer.h"
#include "nsHtml5TreeBuilder.h"
#include "nsHtml5UTF16Buffer.h"
#include "nsHtml5Module.h"


PRBool nsHtml5Module::sEnabled = PR_FALSE;
PRBool nsHtml5Module::sOffMainThread = PR_TRUE;
nsIThread* nsHtml5Module::sStreamParserThread = nsnull;
nsIThread* nsHtml5Module::sMainThread = nsnull;


void
nsHtml5Module::InitializeStatics()
{
  nsContentUtils::AddBoolPrefVarCache("html5.enable", &sEnabled);
  nsContentUtils::AddBoolPrefVarCache("html5.offmainthread", &sOffMainThread);
  NS_NewThread(&sStreamParserThread);
  NS_GetMainThread(&sMainThread);
  nsHtml5Atoms::AddRefAtoms();
  nsHtml5AttributeName::initializeStatics();
  nsHtml5ElementName::initializeStatics();
  nsHtml5HtmlAttributes::initializeStatics();
  nsHtml5NamedCharacters::initializeStatics();
  nsHtml5Portability::initializeStatics();
  nsHtml5StackNode::initializeStatics();
  nsHtml5Tokenizer::initializeStatics();
  nsHtml5TreeBuilder::initializeStatics();
  nsHtml5UTF16Buffer::initializeStatics();
#ifdef DEBUG
  sNsHtml5ModuleInitialized = PR_TRUE;
#endif
}


void
nsHtml5Module::ReleaseStatics()
{
#ifdef DEBUG
  sNsHtml5ModuleInitialized = PR_FALSE;
#endif
  nsHtml5AttributeName::releaseStatics();
  nsHtml5ElementName::releaseStatics();
  nsHtml5HtmlAttributes::releaseStatics();
  nsHtml5NamedCharacters::releaseStatics();
  nsHtml5Portability::releaseStatics();
  nsHtml5StackNode::releaseStatics();
  nsHtml5Tokenizer::releaseStatics();
  nsHtml5TreeBuilder::releaseStatics();
  nsHtml5UTF16Buffer::releaseStatics();
  sStreamParserThread->Shutdown();
  NS_IF_RELEASE(sStreamParserThread);
  NS_IF_RELEASE(sMainThread);
}


already_AddRefed<nsIParser>
nsHtml5Module::NewHtml5Parser()
{
  NS_ABORT_IF_FALSE(sNsHtml5ModuleInitialized, "nsHtml5Module not initialized.");
  nsIParser* rv = static_cast<nsIParser*> (new nsHtml5Parser());
  NS_ADDREF(rv);
  return rv;
}


nsresult
nsHtml5Module::Initialize(nsIParser* aParser, nsIDocument* aDoc, nsIURI* aURI, nsISupports* aContainer, nsIChannel* aChannel)
{
  NS_ABORT_IF_FALSE(sNsHtml5ModuleInitialized, "nsHtml5Module not initialized.");
  nsHtml5Parser* parser = static_cast<nsHtml5Parser*> (aParser);
  return parser->Initialize(aDoc, aURI, aContainer, aChannel);
}


nsIThread*
nsHtml5Module::GetStreamParserThread()
{
  return sOffMainThread ? sStreamParserThread : sMainThread;
}

#ifdef DEBUG
PRBool nsHtml5Module::sNsHtml5ModuleInitialized = PR_FALSE;
#endif
