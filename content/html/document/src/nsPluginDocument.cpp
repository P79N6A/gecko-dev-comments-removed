




































#include "nsMediaDocument.h"
#include "nsIPluginDocument.h"
#include "nsGkAtoms.h"
#include "nsIPresShell.h"
#include "nsIObjectFrame.h"
#include "nsIPluginInstance.h"
#include "nsIDocShellTreeItem.h"
#include "nsNodeInfoManager.h"
#include "nsContentCreatorFunctions.h"
#include "nsContentPolicyUtils.h"
#include "nsIPropertyBag2.h"

class nsPluginDocument : public nsMediaDocument,
                         public nsIPluginDocument
{
public:
  nsPluginDocument();
  virtual ~nsPluginDocument();

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

  void StartLayout() { nsMediaDocument::StartLayout(); }

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(nsPluginDocument, nsMediaDocument)
protected:
  nsresult CreateSyntheticPluginDocument();

  nsCOMPtr<nsIContent>                     mPluginContent;
  nsRefPtr<nsMediaDocumentStreamListener>  mStreamListener;
  nsCString                                mMimeType;

  
  
  
  PRBool                                   mWillHandleInstantiation;
};

class nsPluginStreamListener : public nsMediaDocumentStreamListener
{
public:
  nsPluginStreamListener(nsPluginDocument* doc) :
    nsMediaDocumentStreamListener(doc),  mPluginDoc(doc) {}
  NS_IMETHOD OnStartRequest(nsIRequest* request, nsISupports *ctxt);
private:
  nsresult SetupPlugin();

  nsRefPtr<nsPluginDocument> mPluginDoc;
};


NS_IMETHODIMP
nsPluginStreamListener::OnStartRequest(nsIRequest* request, nsISupports *ctxt)
{
  
  
  nsresult rv = SetupPlugin();

  NS_ASSERTION(NS_FAILED(rv) || mNextStream,
               "We should have a listener by now");
  nsresult rv2 = nsMediaDocumentStreamListener::OnStartRequest(request, ctxt);
  return NS_SUCCEEDED(rv) ? rv2 : rv;
}

nsresult
nsPluginStreamListener::SetupPlugin()
{
  NS_ENSURE_TRUE(mDocument, NS_ERROR_FAILURE);
  mPluginDoc->StartLayout();

  nsCOMPtr<nsIContent> embed = mPluginDoc->GetPluginContent();

  
  nsCOMPtr<nsIPresShell> shell = mDocument->GetPrimaryShell();
  if (!shell) {
    
    mPluginDoc->AllowNormalInstantiation();
    return NS_BINDING_ABORTED;
  }

  
  
  
  shell->FlushPendingNotifications(Flush_Layout);

  nsIFrame* frame = shell->GetPrimaryFrameFor(embed);
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


  
  

nsPluginDocument::nsPluginDocument()
  : mWillHandleInstantiation(PR_TRUE)
{
}

nsPluginDocument::~nsPluginDocument()
{
}

NS_IMPL_CYCLE_COLLECTION_CLASS(nsPluginDocument)

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(nsPluginDocument, nsMediaDocument)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mPluginContent)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(nsPluginDocument, nsMediaDocument)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mPluginContent)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_ISUPPORTS_INHERITED1(nsPluginDocument, nsMediaDocument,
                             nsIPluginDocument)

void
nsPluginDocument::SetScriptGlobalObject(nsIScriptGlobalObject* aScriptGlobalObject)
{
  if (!aScriptGlobalObject) {
    mStreamListener = nsnull;
  }

  nsMediaDocument::SetScriptGlobalObject(aScriptGlobalObject);
}


PRBool
nsPluginDocument::CanSavePresentation(nsIRequest *aNewRequest)
{
  
  
  return PR_FALSE;
}


nsresult
nsPluginDocument::StartDocumentLoad(const char*         aCommand,
                                    nsIChannel*         aChannel,
                                    nsILoadGroup*       aLoadGroup,
                                    nsISupports*        aContainer,
                                    nsIStreamListener** aDocListener,
                                    PRBool              aReset,
                                    nsIContentSink*     aSink)
{
  nsresult rv =
    nsMediaDocument::StartDocumentLoad(aCommand, aChannel, aLoadGroup,
                                       aContainer, aDocListener, aReset,
                                       aSink);
  if (NS_FAILED(rv)) {
    return rv;
  }

  rv = aChannel->GetContentType(mMimeType);
  if (NS_FAILED(rv)) {
    return rv;
  }

  
  rv = CreateSyntheticPluginDocument();
  if (NS_FAILED(rv)) {
    return rv;
  }

  mStreamListener = new nsPluginStreamListener(this);
  if (!mStreamListener) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  NS_ASSERTION(aDocListener, "null aDocListener");
  NS_ADDREF(*aDocListener = mStreamListener);

  return rv;
}

nsresult
nsPluginDocument::CreateSyntheticPluginDocument()
{
  
  
  nsCOMPtr<nsIDocShellTreeItem> dsti (do_QueryReferent(mDocumentContainer));
  if (dsti) {
    PRBool isMsgPane = PR_FALSE;
    dsti->NameEquals(NS_LITERAL_STRING("messagepane").get(), &isMsgPane);
    if (isMsgPane) {
      return NS_ERROR_FAILURE;
    }
  }

  
  nsresult rv = nsMediaDocument::CreateSyntheticDocument();
  NS_ENSURE_SUCCESS(rv, rv);
  

  nsIContent* body = GetBodyContent();
  if (!body) {
    NS_WARNING("no body on plugin document!");
    return NS_ERROR_FAILURE;
  }

  
  NS_NAMED_LITERAL_STRING(zero, "0");
  body->SetAttr(kNameSpaceID_None, nsGkAtoms::marginwidth, zero, PR_FALSE);
  body->SetAttr(kNameSpaceID_None, nsGkAtoms::marginheight, zero, PR_FALSE);


  
  nsCOMPtr<nsINodeInfo> nodeInfo;
  nodeInfo = mNodeInfoManager->GetNodeInfo(nsGkAtoms::embed, nsnull,
                                           kNameSpaceID_XHTML);
  NS_ENSURE_TRUE(nodeInfo, NS_ERROR_OUT_OF_MEMORY);
  rv = NS_NewHTMLElement(getter_AddRefs(mPluginContent), nodeInfo, PR_FALSE);
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
nsPluginDocument::SetStreamListener(nsIStreamListener *aListener)
{
  if (mStreamListener) {
    mStreamListener->SetStreamListener(aListener);
  }

  nsMediaDocument::UpdateTitleAndCharset(mMimeType);

  return NS_OK;
}

NS_IMETHODIMP
nsPluginDocument::Print()
{
  NS_ENSURE_TRUE(mPluginContent, NS_ERROR_FAILURE);

  nsIPresShell *shell = GetPrimaryShell();
  if (!shell) {
    return NS_OK;
  }

  nsIFrame* frame = shell->GetPrimaryFrameFor(mPluginContent);
  NS_ENSURE_TRUE(frame, NS_ERROR_FAILURE);

  nsIObjectFrame* objectFrame = do_QueryFrame(frame);
  if (objectFrame) {
    nsCOMPtr<nsIPluginInstance> pi;
    objectFrame->GetPluginInstance(*getter_AddRefs(pi));

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
nsPluginDocument::GetWillHandleInstantiation(PRBool* aWillHandle)
{
  *aWillHandle = mWillHandleInstantiation;
  return NS_OK;
}

nsresult
NS_NewPluginDocument(nsIDocument** aResult)
{
  nsPluginDocument* doc = new nsPluginDocument();
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
