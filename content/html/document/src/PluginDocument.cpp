




#include "MediaDocument.h"
#include "nsIPluginDocument.h"
#include "nsGkAtoms.h"
#include "nsIPresShell.h"
#include "nsIObjectFrame.h"
#include "nsNPAPIPluginInstance.h"
#include "nsIDocumentInlines.h"
#include "nsIDocShellTreeItem.h"
#include "nsNodeInfoManager.h"
#include "nsContentCreatorFunctions.h"
#include "nsContentPolicyUtils.h"
#include "nsIPropertyBag2.h"
#include "mozilla/dom/Element.h"
#include "nsObjectLoadingContent.h"
#include "GeckoProfiler.h"

namespace mozilla {
namespace dom {

class PluginDocument MOZ_FINAL : public MediaDocument
                               , public nsIPluginDocument
{
public:
  PluginDocument();

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIPLUGINDOCUMENT

  virtual nsresult StartDocumentLoad(const char*         aCommand,
                                     nsIChannel*         aChannel,
                                     nsILoadGroup*       aLoadGroup,
                                     nsISupports*        aContainer,
                                     nsIStreamListener** aDocListener,
                                     bool                aReset = true,
                                     nsIContentSink*     aSink = nullptr);

  virtual void SetScriptGlobalObject(nsIScriptGlobalObject* aScriptGlobalObject);
  virtual bool CanSavePresentation(nsIRequest *aNewRequest);

  const nsCString& GetType() const { return mMimeType; }
  Element*         GetPluginContent() { return mPluginContent; }

  void StartLayout() { MediaDocument::StartLayout(); }

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(PluginDocument, MediaDocument)
protected:
  virtual ~PluginDocument();

  nsresult CreateSyntheticPluginDocument();

  nsCOMPtr<Element>                        mPluginContent;
  nsRefPtr<MediaDocumentStreamListener>    mStreamListener;
  nsCString                                mMimeType;
};

class PluginStreamListener : public MediaDocumentStreamListener
{
public:
  PluginStreamListener(PluginDocument* doc)
    : MediaDocumentStreamListener(doc)
    , mPluginDoc(doc)
  {}
  NS_IMETHOD OnStartRequest(nsIRequest* request, nsISupports *ctxt);
private:
  nsRefPtr<PluginDocument> mPluginDoc;
};


NS_IMETHODIMP
PluginStreamListener::OnStartRequest(nsIRequest* request, nsISupports *ctxt)
{
  PROFILER_LABEL("PluginStreamListener", "OnStartRequest",
    js::ProfileEntry::Category::NETWORK);

  nsCOMPtr<nsIContent> embed = mPluginDoc->GetPluginContent();
  nsCOMPtr<nsIObjectLoadingContent> objlc = do_QueryInterface(embed);
  nsCOMPtr<nsIStreamListener> objListener = do_QueryInterface(objlc);

  if (!objListener) {
    NS_NOTREACHED("PluginStreamListener without appropriate content node");
    return NS_BINDING_ABORTED;
  }

  SetStreamListener(objListener);

  
  
  nsresult rv = objlc->InitializeFromChannel(request);
  if (NS_FAILED(rv)) {
    NS_NOTREACHED("InitializeFromChannel failed");
    return rv;
  }

  
  
  return MediaDocumentStreamListener::OnStartRequest(request, ctxt);
}

  
  

PluginDocument::PluginDocument()
{}

PluginDocument::~PluginDocument()
{}


NS_IMPL_CYCLE_COLLECTION_INHERITED(PluginDocument, MediaDocument,
                                   mPluginContent)

NS_IMPL_ADDREF_INHERITED(PluginDocument, MediaDocument)
NS_IMPL_RELEASE_INHERITED(PluginDocument, MediaDocument)

NS_INTERFACE_TABLE_HEAD_CYCLE_COLLECTION_INHERITED(PluginDocument)
  NS_INTERFACE_TABLE_INHERITED(PluginDocument, nsIPluginDocument)
NS_INTERFACE_TABLE_TAIL_INHERITING(MediaDocument)

void
PluginDocument::SetScriptGlobalObject(nsIScriptGlobalObject* aScriptGlobalObject)
{
  
  
  MediaDocument::SetScriptGlobalObject(aScriptGlobalObject);

  if (aScriptGlobalObject) {
    if (!mPluginContent) {
      
#ifdef DEBUG
      nsresult rv =
#endif
        CreateSyntheticPluginDocument();
      NS_ASSERTION(NS_SUCCEEDED(rv), "failed to create synthetic document");
    }
    BecomeInteractive();
  } else {
    mStreamListener = nullptr;
  }
}


bool
PluginDocument::CanSavePresentation(nsIRequest *aNewRequest)
{
  
  
  return false;
}


nsresult
PluginDocument::StartDocumentLoad(const char*         aCommand,
                                  nsIChannel*         aChannel,
                                  nsILoadGroup*       aLoadGroup,
                                  nsISupports*        aContainer,
                                  nsIStreamListener** aDocListener,
                                  bool                aReset,
                                  nsIContentSink*     aSink)
{
  
  
  nsCOMPtr<nsIDocShellTreeItem> dsti (do_QueryInterface(aContainer));
  if (dsti) {
    bool isMsgPane = false;
    dsti->NameEquals(MOZ_UTF16("messagepane"), &isMsgPane);
    if (isMsgPane) {
      return NS_ERROR_FAILURE;
    }
  }

  nsresult rv =
    MediaDocument::StartDocumentLoad(aCommand, aChannel, aLoadGroup, aContainer,
                                     aDocListener, aReset, aSink);
  if (NS_FAILED(rv)) {
    return rv;
  }

  rv = aChannel->GetContentType(mMimeType);
  if (NS_FAILED(rv)) {
    return rv;
  }

  MediaDocument::UpdateTitleAndCharset(mMimeType);

  mStreamListener = new PluginStreamListener(this);
  NS_ASSERTION(aDocListener, "null aDocListener");
  NS_ADDREF(*aDocListener = mStreamListener);

  return rv;
}

nsresult
PluginDocument::CreateSyntheticPluginDocument()
{
  NS_ASSERTION(!GetShell() || !GetShell()->DidInitialize(),
               "Creating synthetic plugin document content too late");

  
  nsresult rv = MediaDocument::CreateSyntheticDocument();
  NS_ENSURE_SUCCESS(rv, rv);
  

  Element* body = GetBodyElement();
  if (!body) {
    NS_WARNING("no body on plugin document!");
    return NS_ERROR_FAILURE;
  }

  
  NS_NAMED_LITERAL_STRING(zero, "0");
  body->SetAttr(kNameSpaceID_None, nsGkAtoms::marginwidth, zero, false);
  body->SetAttr(kNameSpaceID_None, nsGkAtoms::marginheight, zero, false);


  
  nsRefPtr<mozilla::dom::NodeInfo> nodeInfo;
  nodeInfo = mNodeInfoManager->GetNodeInfo(nsGkAtoms::embed, nullptr,
                                           kNameSpaceID_XHTML,
                                           nsIDOMNode::ELEMENT_NODE);
  rv = NS_NewHTMLElement(getter_AddRefs(mPluginContent), nodeInfo.forget(),
                         NOT_FROM_PARSER);
  NS_ENSURE_SUCCESS(rv, rv);

  
  mPluginContent->SetAttr(kNameSpaceID_None, nsGkAtoms::name,
                          NS_LITERAL_STRING("plugin"), false);

  
  NS_NAMED_LITERAL_STRING(percent100, "100%");
  mPluginContent->SetAttr(kNameSpaceID_None, nsGkAtoms::width, percent100,
                          false);
  mPluginContent->SetAttr(kNameSpaceID_None, nsGkAtoms::height, percent100,
                          false);

  
  nsAutoCString src;
  mDocumentURI->GetSpec(src);
  mPluginContent->SetAttr(kNameSpaceID_None, nsGkAtoms::src,
                          NS_ConvertUTF8toUTF16(src), false);

  
  mPluginContent->SetAttr(kNameSpaceID_None, nsGkAtoms::type,
                          NS_ConvertUTF8toUTF16(mMimeType), false);

  
  
  body->AppendChildTo(mPluginContent, false);

  return NS_OK;


}

NS_IMETHODIMP
PluginDocument::Print()
{
  NS_ENSURE_TRUE(mPluginContent, NS_ERROR_FAILURE);

  nsIObjectFrame* objectFrame =
    do_QueryFrame(mPluginContent->GetPrimaryFrame());
  if (objectFrame) {
    nsRefPtr<nsNPAPIPluginInstance> pi;
    objectFrame->GetPluginInstance(getter_AddRefs(pi));
    if (pi) {
      NPPrint npprint;
      npprint.mode = NP_FULL;
      npprint.print.fullPrint.pluginPrinted = false;
      npprint.print.fullPrint.printOne = false;
      npprint.print.fullPrint.platformPrint = nullptr;

      pi->Print(&npprint);
    }
  }

  return NS_OK;
}

} 
} 

nsresult
NS_NewPluginDocument(nsIDocument** aResult)
{
  mozilla::dom::PluginDocument* doc = new mozilla::dom::PluginDocument();

  NS_ADDREF(doc);
  nsresult rv = doc->Init();

  if (NS_FAILED(rv)) {
    NS_RELEASE(doc);
  }

  *aResult = doc;

  return rv;
}
