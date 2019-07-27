





#include "nsHtml5DocumentBuilder.h"

#include "nsIStyleSheetLinkingElement.h"
#include "nsStyleLinkElement.h"
#include "nsScriptLoader.h"
#include "nsIHTMLDocument.h"

NS_IMPL_CYCLE_COLLECTION_INHERITED(nsHtml5DocumentBuilder, nsContentSink,
                                   mOwnedElements)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(nsHtml5DocumentBuilder)
NS_INTERFACE_MAP_END_INHERITING(nsContentSink)

NS_IMPL_ADDREF_INHERITED(nsHtml5DocumentBuilder, nsContentSink)
NS_IMPL_RELEASE_INHERITED(nsHtml5DocumentBuilder, nsContentSink)

nsHtml5DocumentBuilder::nsHtml5DocumentBuilder(bool aRunsToCompletion)
{
  mRunsToCompletion = aRunsToCompletion;
}

nsresult
nsHtml5DocumentBuilder::Init(nsIDocument* aDoc,
                            nsIURI* aURI,
                            nsISupports* aContainer,
                            nsIChannel* aChannel)
{
  return nsContentSink::Init(aDoc, aURI, aContainer, aChannel);
}

nsHtml5DocumentBuilder::~nsHtml5DocumentBuilder()
{
}

nsresult
nsHtml5DocumentBuilder::MarkAsBroken(nsresult aReason)
{
  mBroken = aReason;
  return aReason;
}

void
nsHtml5DocumentBuilder::SetDocumentCharsetAndSource(nsACString& aCharset, int32_t aCharsetSource)
{
  if (mDocument) {
    mDocument->SetDocumentCharacterSetSource(aCharsetSource);
    mDocument->SetDocumentCharacterSet(aCharset);
  }
}

void
nsHtml5DocumentBuilder::UpdateStyleSheet(nsIContent* aElement)
{
  
  
  EndDocUpdate();

  if (MOZ_UNLIKELY(!mParser)) {
    
    return;
  }

  nsCOMPtr<nsIStyleSheetLinkingElement> ssle(do_QueryInterface(aElement));
  NS_ASSERTION(ssle, "Node didn't QI to style.");

  ssle->SetEnableUpdates(true);

  bool willNotify;
  bool isAlternate;
  nsresult rv = ssle->UpdateStyleSheet(mRunsToCompletion ? nullptr : this,
                                       &willNotify,
                                       &isAlternate);
  if (NS_SUCCEEDED(rv) && willNotify && !isAlternate && !mRunsToCompletion) {
    ++mPendingSheetCount;
    mScriptLoader->AddExecuteBlocker();
  }

  if (aElement->IsHTMLElement(nsGkAtoms::link)) {
    
    nsAutoString relVal;
    aElement->GetAttr(kNameSpaceID_None, nsGkAtoms::rel, relVal);
    if (!relVal.IsEmpty()) {
      uint32_t linkTypes =
        nsStyleLinkElement::ParseLinkTypes(relVal, aElement->NodePrincipal());
      bool hasPrefetch = linkTypes & nsStyleLinkElement::ePREFETCH;
      if (hasPrefetch || (linkTypes & nsStyleLinkElement::eNEXT)) {
        nsAutoString hrefVal;
        aElement->GetAttr(kNameSpaceID_None, nsGkAtoms::href, hrefVal);
        if (!hrefVal.IsEmpty()) {
          PrefetchHref(hrefVal, aElement, hasPrefetch);
        }
      }
      if (linkTypes & nsStyleLinkElement::eDNS_PREFETCH) {
        nsAutoString hrefVal;
        aElement->GetAttr(kNameSpaceID_None, nsGkAtoms::href, hrefVal);
        if (!hrefVal.IsEmpty()) {
          PrefetchDNS(hrefVal);
        }
      }
    }
  }

  
  BeginDocUpdate();
}

void
nsHtml5DocumentBuilder::SetDocumentMode(nsHtml5DocumentMode m)
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



void
nsHtml5DocumentBuilder::UpdateChildCounts()
{
  
}

nsresult
nsHtml5DocumentBuilder::FlushTags()
{
  return NS_OK;
}
