




































#include "MediaDocument.h"
#include "nsIPluginDocument.h"
#include "nsGkAtoms.h"
#include "nsIPresShell.h"
#include "nsIObjectFrame.h"
#include "nsNPAPIPluginInstance.h"
#include "nsIDocShellTreeItem.h"
#include "nsNodeInfoManager.h"
#include "nsContentCreatorFunctions.h"
#include "nsContentPolicyUtils.h"
#include "nsIPropertyBag2.h"
#include "mozilla/dom/Element.h"

namespace mozilla {
namespace dom {

class PluginDocument : public MediaDocument
                     , public nsIPluginDocument
{
public:
  PluginDocument();
  virtual ~PluginDocument();

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIPLUGINDOCUMENT

  virtual nsresult StartDocumentLoad(const char*         aCommand,
                                     nsIChannel*         aChannel,
                                     nsILoadGroup*       aLoadGroup,
                                     nsISupports*        aContainer,
                                     nsIStreamListener** aDocListener,
                                     PRBool              aReset = PR_TRUE,
                                     nsIContentSink*     aSink = nsnull);

  virtual void SetScriptGlobalObject(nsIScriptGlobalObject* aScriptGlobalObject);
  virtual PRBool CanSavePresentation(nsIRequest *aNewRequest);

  const nsCString& GetType() const { return mMimeType; }
  nsIContent*      GetPluginContent() { return mPluginContent; }

  void AllowNormalInstantiation() {
    mWillHandleInstantiation = PR_FALSE;
  }

  void StartLayout() { MediaDocument::StartLayout(); }

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(PluginDocument, MediaDocument)
protected:
  nsresult CreateSyntheticPluginDocument();

  nsCOMPtr<nsIContent>                     mPluginContent;
  nsRefPtr<MediaDocumentStreamListener>    mStreamListener;
  nsCString                                mMimeType;

  
  
  
  PRBool                                   mWillHandleInstantiation;
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
  nsresult SetupPlugin();

  nsRefPtr<PluginDocument> mPluginDoc;
};


NS_IMETHODIMP
PluginStreamListener::OnStartRequest(nsIRequest* request, nsISupports *ctxt)
{
  
  
  nsresult rv = SetupPlugin();

  NS_ASSERTION(NS_FAILED(rv) || mNextStream,
               "We should have a listener by now");
  nsresult rv2 = MediaDocumentStreamListener::OnStartRequest(request, ctxt);
  return NS_SUCCEEDED(rv) ? rv2 : rv;
}

nsresult
PluginStreamListener::SetupPlugin()
{
  NS_ENSURE_TRUE(mDocument, NS_ERROR_FAILURE);
  mPluginDoc->StartLayout();

  nsCOMPtr<nsIContent> embed = mPluginDoc->GetPluginContent();

  
  nsCOMPtr<nsIPresShell> shell = mDocument->GetShell();
  if (!shell) {
    
    mPluginDoc->AllowNormalInstantiation();
    return NS_BINDING_ABORTED;
  }

  
  
  
  shell->FlushPendingNotifications(Flush_Layout);

  nsIFrame* frame = embed->GetPrimaryFrame();
  if (!frame) {
    mPluginDoc->AllowNormalInstantiation();
    return NS_OK;
  }

  nsIObjectFrame* objFrame = do_QueryFrame(frame);
  if (!objFrame) {
    mPluginDoc->AllowNormalInstantiation();
    return NS_ERROR_UNEXPECTED;
  }

  nsresult rv = objFrame->Instantiate(mPluginDoc->GetType().get(),
                                      mDocument->nsIDocument::GetDocumentURI());
  if (NS_FAILED(rv)) {
    return rv;
  }

  
  
  mPluginDoc->AllowNormalInstantiation();

  return NS_OK;
}


  
  

PluginDocument::PluginDocument()
  : mWillHandleInstantiation(PR_TRUE)
{
}

PluginDocument::~PluginDocument()
{
}

NS_IMPL_CYCLE_COLLECTION_CLASS(PluginDocument)

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(PluginDocument, MediaDocument)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mPluginContent)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(PluginDocument, MediaDocument)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mPluginContent)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_ADDREF_INHERITED(PluginDocument, MediaDocument)
NS_IMPL_RELEASE_INHERITED(PluginDocument, MediaDocument)

NS_INTERFACE_TABLE_HEAD_CYCLE_COLLECTION_INHERITED(PluginDocument)
  NS_INTERFACE_TABLE_INHERITED1(PluginDocument, nsIPluginDocument)
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
  } else {
    mStreamListener = nsnull;
  }
}


PRBool
PluginDocument::CanSavePresentation(nsIRequest *aNewRequest)
{
  
  
  return PR_FALSE;
}


nsresult
PluginDocument::StartDocumentLoad(const char*         aCommand,
                                  nsIChannel*         aChannel,
                                  nsILoadGroup*       aLoadGroup,
                                  nsISupports*        aContainer,
                                  nsIStreamListener** aDocListener,
                                  PRBool              aReset,
                                  nsIContentSink*     aSink)
{
  
  
  nsCOMPtr<nsIDocShellTreeItem> dsti (do_QueryInterface(aContainer));
  if (dsti) {
    PRBool isMsgPane = PR_FALSE;
    dsti->NameEquals(NS_LITERAL_STRING("messagepane").get(), &isMsgPane);
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

  mStreamListener = new PluginStreamListener(this);
  if (!mStreamListener) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  NS_ASSERTION(aDocListener, "null aDocListener");
  NS_ADDREF(*aDocListener = mStreamListener);

  return rv;
}

nsresult
PluginDocument::CreateSyntheticPluginDocument()
{
  NS_ASSERTION(!GetShell() || !GetShell()->DidInitialReflow(),
               "Creating synthetic plugin document content too late");

  
  nsresult rv = MediaDocument::CreateSyntheticDocument();
  NS_ENSURE_SUCCESS(rv, rv);
  

  Element* body = GetBodyElement();
  if (!body) {
    NS_WARNING("no body on plugin document!");
    return NS_ERROR_FAILURE;
  }

  
  NS_NAMED_LITERAL_STRING(zero, "0");
  body->SetAttr(kNameSpaceID_None, nsGkAtoms::marginwidth, zero, PR_FALSE);
  body->SetAttr(kNameSpaceID_None, nsGkAtoms::marginheight, zero, PR_FALSE);


  
  nsCOMPtr<nsINodeInfo> nodeInfo;
  nodeInfo = mNodeInfoManager->GetNodeInfo(nsGkAtoms::embed, nsnull,
                                           kNameSpaceID_XHTML,
                                           nsIDOMNode::ELEMENT_NODE);
  NS_ENSURE_TRUE(nodeInfo, NS_ERROR_OUT_OF_MEMORY);
  rv = NS_NewHTMLElement(getter_AddRefs(mPluginContent), nodeInfo.forget(),
                         NOT_FROM_PARSER);
  NS_ENSURE_SUCCESS(rv, rv);

  
  mPluginContent->SetAttr(kNameSpaceID_None, nsGkAtoms::name,
                          NS_LITERAL_STRING("plugin"), PR_FALSE);

  
  NS_NAMED_LITERAL_STRING(percent100, "100%");
  mPluginContent->SetAttr(kNameSpaceID_None, nsGkAtoms::width, percent100,
                          PR_FALSE);
  mPluginContent->SetAttr(kNameSpaceID_None, nsGkAtoms::height, percent100,
                          PR_FALSE);

  
  nsCAutoString src;
  mDocumentURI->GetSpec(src);
  mPluginContent->SetAttr(kNameSpaceID_None, nsGkAtoms::src,
                          NS_ConvertUTF8toUTF16(src), PR_FALSE);

  
  mPluginContent->SetAttr(kNameSpaceID_None, nsGkAtoms::type,
                          NS_ConvertUTF8toUTF16(mMimeType), PR_FALSE);

  
  
  body->AppendChildTo(mPluginContent, PR_FALSE);

  return NS_OK;


}

NS_IMETHODIMP
PluginDocument::SetStreamListener(nsIStreamListener *aListener)
{
  if (mStreamListener) {
    mStreamListener->SetStreamListener(aListener);
  }

  MediaDocument::UpdateTitleAndCharset(mMimeType);

  return NS_OK;
}

NS_IMETHODIMP
PluginDocument::Print()
{
  NS_ENSURE_TRUE(mPluginContent, NS_ERROR_FAILURE);

  nsIObjectFrame* objectFrame =
    do_QueryFrame(mPluginContent->GetPrimaryFrame());
  if (objectFrame) {
    nsCOMPtr<nsNPAPIPluginInstance> pi;
    objectFrame->GetPluginInstance(getter_AddRefs(pi));
    if (pi) {
      NPPrint npprint;
      npprint.mode = NP_FULL;
      npprint.print.fullPrint.pluginPrinted = PR_FALSE;
      npprint.print.fullPrint.printOne = PR_FALSE;
      npprint.print.fullPrint.platformPrint = nsnull;

      pi->Print(&npprint);
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
PluginDocument::GetWillHandleInstantiation(PRBool* aWillHandle)
{
  *aWillHandle = mWillHandleInstantiation;
  return NS_OK;
}

} 
} 

nsresult
NS_NewPluginDocument(nsIDocument** aResult)
{
  mozilla::dom::PluginDocument* doc = new mozilla::dom::PluginDocument();
  if (!doc) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  NS_ADDREF(doc);
  nsresult rv = doc->Init();

  if (NS_FAILED(rv)) {
    NS_RELEASE(doc);
  }

  *aResult = doc;

  return rv;
}
