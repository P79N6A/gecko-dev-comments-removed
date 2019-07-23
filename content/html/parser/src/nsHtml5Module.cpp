




































#include "nsHtml5AttributeName.h"
#include "nsHtml5ElementName.h"
#include "nsHtml5HtmlAttributes.h"
#include "nsHtml5NamedCharacters.h"
#include "nsHtml5Portability.h"
#include "nsHtml5StackNode.h"
#include "nsHtml5StringLiterals.h"
#include "nsHtml5Tokenizer.h"
#include "nsHtml5TreeBuilder.h"
#include "nsHtml5UTF16Buffer.h"

#include "nsHtml5Module.h"


void
nsHtml5Module::InitializeStatics()
{
  nsHtml5Atoms::AddRefAtoms();
  nsHtml5AttributeName::initializeStatics();
  nsHtml5ElementName::initializeStatics();
  nsHtml5HtmlAttributes::initializeStatics();
  nsHtml5NamedCharacters::initializeStatics();
  nsHtml5Portability::initializeStatics();
  nsHtml5StackNode::initializeStatics();
  nsHtml5StringLiterals::initializeStatics();
  nsHtml5Tokenizer::initializeStatics();
  nsHtml5TreeBuilder::initializeStatics();
  nsHtml5UTF16Buffer::initializeStatics();
}


void
nsHtml5Module::ReleaseStatics()
{
  nsHtml5AttributeName::releaseStatics();
  nsHtml5ElementName::releaseStatics();
  nsHtml5HtmlAttributes::releaseStatics();
  nsHtml5NamedCharacters::releaseStatics();
  nsHtml5Portability::releaseStatics();
  nsHtml5StackNode::releaseStatics();
  nsHtml5StringLiterals::releaseStatics();
  nsHtml5Tokenizer::releaseStatics();
  nsHtml5TreeBuilder::releaseStatics();
  nsHtml5UTF16Buffer::releaseStatics();
}


already_AddRefed<nsIParser> 
nsHtml5Module::NewHtml5Parser()
{
  nsIParser* rv = static_cast<nsIParser*> (new nsHtml5Parser());
  NS_ADDREF(rv);
  return rv;
}



nsresult 
nsHtml5Module::Initialize(nsIParser* aParser, nsIDocument* aDoc, nsIURI* aURI, nsISupports* aContainer, nsIChannel* aChannel)
{
  nsHtml5Parser* parser = static_cast<nsHtml5Parser*> (aParser);
  return parser->Initialize(aDoc, aURI, aContainer, aChannel);
}

