





#include "nsHtml5OplessBuilder.h"

#include "nsScriptLoader.h"
#include "mozilla/css/Loader.h"
#include "nsIDocShell.h"
#include "nsIHTMLDocument.h"

nsHtml5OplessBuilder::nsHtml5OplessBuilder()
 : nsHtml5DocumentBuilder(true)
{
}

nsHtml5OplessBuilder::~nsHtml5OplessBuilder()
{
}

void
nsHtml5OplessBuilder::Start()
{
  mFlushState = eInFlush;
  BeginDocUpdate();
}

void
nsHtml5OplessBuilder::Finish()
{
  EndDocUpdate();
  DropParserAndPerfHint();
  mScriptLoader = nullptr;
  mDocument = nullptr;
  mNodeInfoManager = nullptr;
  mCSSLoader = nullptr;
  mDocumentURI = nullptr;
  mDocShell = nullptr;
  mOwnedElements.Clear();
  mFlushState = eNotFlushing;
}

void
nsHtml5OplessBuilder::SetParser(nsParserBase* aParser)
{
  mParser = aParser;
}
