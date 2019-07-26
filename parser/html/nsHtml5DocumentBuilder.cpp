





#include "nsHtml5DocumentBuilder.h"

#include "nsScriptLoader.h"
#include "mozilla/css/Loader.h"
#include "nsIDocShell.h"

NS_IMPL_CYCLE_COLLECTION_INHERITED_1(nsHtml5DocumentBuilder, nsContentSink,
                                     mOwnedElements)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(nsHtml5DocumentBuilder)
NS_INTERFACE_MAP_END_INHERITING(nsContentSink)

NS_IMPL_ADDREF_INHERITED(nsHtml5DocumentBuilder, nsContentSink)
NS_IMPL_RELEASE_INHERITED(nsHtml5DocumentBuilder, nsContentSink)

void
nsHtml5DocumentBuilder::DropHeldElements()
{
  mScriptLoader = nullptr;
  mDocument = nullptr;
  mNodeInfoManager = nullptr;
  mCSSLoader = nullptr;
  mDocumentURI = nullptr;
  mDocShell = nullptr;
  mOwnedElements.Clear();
}

