





































#include "nsMediaDocument.h"
#include "nsGkAtoms.h"
#include "nsRect.h"
#include "nsPresContext.h"
#include "nsIPresShell.h"
#include "nsIScrollable.h"
#include "nsIViewManager.h"
#include "nsITextToSubURI.h"
#include "nsIURL.h"
#include "nsPrintfCString.h"
#include "nsIContentViewer.h"
#include "nsIMarkupDocumentViewer.h"
#include "nsIDocShell.h"
#include "nsIParser.h" 
#include "nsIDocumentCharsetInfo.h" 
#include "nsNodeInfoManager.h"

nsMediaDocumentStreamListener::nsMediaDocumentStreamListener(nsMediaDocument *aDocument)
{
  mDocument = aDocument;
}

nsMediaDocumentStreamListener::~nsMediaDocumentStreamListener()
{
}


NS_IMPL_THREADSAFE_ISUPPORTS2(nsMediaDocumentStreamListener,
                              nsIRequestObserver,
                              nsIStreamListener)


void
nsMediaDocumentStreamListener::SetStreamListener(nsIStreamListener *aListener)
{
  mNextStream = aListener;
}

NS_IMETHODIMP
nsMediaDocumentStreamListener::OnStartRequest(nsIRequest* request, nsISupports *ctxt)
{
  NS_ENSURE_TRUE(mDocument, NS_ERROR_FAILURE);

  mDocument->StartLayout();

  if (mNextStream) {
    return mNextStream->OnStartRequest(request, ctxt);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsMediaDocumentStreamListener::OnStopRequest(nsIRequest* request,
                                             nsISupports *ctxt,
                                             nsresult status)
{
  nsresult rv = NS_OK;
  if (mNextStream) {
    rv = mNextStream->OnStopRequest(request, ctxt, status);
  }

  
  mDocument = nsnull;

  return rv;
}

NS_IMETHODIMP
nsMediaDocumentStreamListener::OnDataAvailable(nsIRequest* request,
                                               nsISupports *ctxt,
                                               nsIInputStream *inStr,
                                               PRUint32 sourceOffset,
                                               PRUint32 count)
{
  if (mNextStream) {
    return mNextStream->OnDataAvailable(request, ctxt, inStr, sourceOffset, count);
  }

  return NS_OK;
}


const char* const nsMediaDocument::sFormatNames[4] = 
{
  "MediaTitleWithNoInfo",    
  "MediaTitleWithFile",      
  "",                        
  ""                         
};

nsMediaDocument::nsMediaDocument()
{
}
nsMediaDocument::~nsMediaDocument()
{
}

nsresult
nsMediaDocument::Init()
{
  nsresult rv = nsHTMLDocument::Init();
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsIStringBundleService> stringService(
    do_GetService(NS_STRINGBUNDLE_CONTRACTID));
  if (stringService) {
    stringService->CreateBundle(NSMEDIADOCUMENT_PROPERTIES_URI,
                                getter_AddRefs(mStringBundle));
  }

  return NS_OK;
}

nsresult
nsMediaDocument::StartDocumentLoad(const char*         aCommand,
                                   nsIChannel*         aChannel,
                                   nsILoadGroup*       aLoadGroup,
                                   nsISupports*        aContainer,
                                   nsIStreamListener** aDocListener,
                                   PRBool              aReset,
                                   nsIContentSink*     aSink)
{
  nsresult rv = nsDocument::StartDocumentLoad(aCommand, aChannel, aLoadGroup,
                                              aContainer, aDocListener, aReset,
                                              aSink);
  if (NS_FAILED(rv)) {
    return rv;
  }

  
  
  
  
  
  

  
  
  
  
  
  
  
  
  
  
  
    
  nsCOMPtr<nsIDocShell> docShell(do_QueryInterface(aContainer));

  
  NS_ENSURE_TRUE(docShell, NS_OK); 

  nsCOMPtr<nsIDocumentCharsetInfo> dcInfo;
  nsCAutoString charset;

  docShell->GetDocumentCharsetInfo(getter_AddRefs(dcInfo));
  if (dcInfo) {
    nsCOMPtr<nsIAtom> csAtom;
    dcInfo->GetParentCharset(getter_AddRefs(csAtom));
    if (csAtom) {   
      csAtom->ToUTF8String(charset);
    }
  }

  if (charset.IsEmpty() || charset.Equals("UTF-8")) {
    nsCOMPtr<nsIContentViewer> cv;
    docShell->GetContentViewer(getter_AddRefs(cv));

    
    NS_ENSURE_TRUE(cv, NS_OK); 
    nsCOMPtr<nsIMarkupDocumentViewer> muCV = do_QueryInterface(cv);
    if (muCV) {
      muCV->GetPrevDocCharacterSet(charset);   
      if (charset.Equals("UTF-8") || charset.IsEmpty()) {
        muCV->GetDefaultCharacterSet(charset); 
      }
    } 
  }

  if (!charset.IsEmpty() && !charset.Equals("UTF-8")) {
    SetDocumentCharacterSet(charset);
    mCharacterSetSource = kCharsetFromUserDefault;
  }

  return NS_OK;
}

nsresult
nsMediaDocument::CreateSyntheticDocument()
{
  
  nsresult rv;

  nsCOMPtr<nsINodeInfo> nodeInfo;
  rv = mNodeInfoManager->GetNodeInfo(nsGkAtoms::html, nsnull,
                                     kNameSpaceID_None,
                                     getter_AddRefs(nodeInfo));
  NS_ENSURE_SUCCESS(rv, rv);

  nsRefPtr<nsGenericHTMLElement> root = NS_NewHTMLHtmlElement(nodeInfo);
  if (!root) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  NS_ASSERTION(GetChildCount() == 0, "Shouldn't have any kids");
  rv = AppendChildTo(root, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mNodeInfoManager->GetNodeInfo(nsGkAtoms::body, nsnull,
                                     kNameSpaceID_None,
                                     getter_AddRefs(nodeInfo));
  NS_ENSURE_SUCCESS(rv, rv);

  nsRefPtr<nsGenericHTMLElement> body = NS_NewHTMLBodyElement(nodeInfo);
  if (!body) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  mBodyContent = do_QueryInterface(body);

  root->AppendChildTo(body, PR_FALSE);

  return NS_OK;
}

nsresult
nsMediaDocument::StartLayout()
{
  nsPresShellIterator iter(this);
  nsCOMPtr<nsIPresShell> shell;
  while ((shell = iter.GetNextShell())) {
    
    shell->BeginObservingDocument();

    
    nsRect visibleArea = shell->GetPresContext()->GetVisibleArea();
    nsCOMPtr<nsIPresShell> shellGrip = shell;
    nsresult rv = shell->InitialReflow(visibleArea.width, visibleArea.height);
    NS_ENSURE_SUCCESS(rv, rv);

    
    
    nsIViewManager* vm = shell->GetViewManager();
    if (vm) {
      vm->EnableRefresh(NS_VMREFRESH_IMMEDIATE);
    }
  }

  return NS_OK;
}

void 
nsMediaDocument::UpdateTitleAndCharset(const nsACString& aTypeStr,
                                       const char* const* aFormatNames,
                                       PRInt32 aWidth, PRInt32 aHeight,
                                       const nsAString& aStatus)
{
  nsXPIDLString fileStr;
  if (mDocumentURI) {
    nsCAutoString fileName;
    nsCOMPtr<nsIURL> url = do_QueryInterface(mDocumentURI);
    if (url)
      url->GetFileName(fileName);

    nsCAutoString docCharset;

    
    
    
    
    
    
    if (mCharacterSetSource != kCharsetUninitialized) {  
      docCharset = mCharacterSet;
    }
    else {  
      
      mDocumentURI->GetOriginCharset(docCharset);
      SetDocumentCharacterSet(docCharset);
    }
    if (!fileName.IsEmpty()) {
      nsresult rv;
      nsCOMPtr<nsITextToSubURI> textToSubURI = 
        do_GetService(NS_ITEXTTOSUBURI_CONTRACTID, &rv);
      if (NS_SUCCEEDED(rv))
        
        textToSubURI->UnEscapeURIForUI(docCharset, fileName, fileStr);
      else 
        CopyUTF8toUTF16(fileName, fileStr);
    }
  }


  NS_ConvertASCIItoUTF16 typeStr(aTypeStr);
  nsXPIDLString title;

  if (mStringBundle) {
    
    if (aWidth != 0 && aHeight != 0) {
      nsAutoString widthStr;
      nsAutoString heightStr;
      widthStr.AppendInt(aWidth);
      heightStr.AppendInt(aHeight);
      
      if (!fileStr.IsEmpty()) {
        const PRUnichar *formatStrings[4]  = {fileStr.get(), typeStr.get(), 
          widthStr.get(), heightStr.get()};
        NS_ConvertASCIItoUTF16 fmtName(aFormatNames[eWithDimAndFile]);
        mStringBundle->FormatStringFromName(fmtName.get(), formatStrings, 4,
                                            getter_Copies(title));
      } 
      else {
        const PRUnichar *formatStrings[3]  = {typeStr.get(), widthStr.get(), 
          heightStr.get()};
        NS_ConvertASCIItoUTF16 fmtName(aFormatNames[eWithDim]);
        mStringBundle->FormatStringFromName(fmtName.get(), formatStrings, 3,
                                            getter_Copies(title));
      }
    } 
    else {
    
      if (!fileStr.IsEmpty()) {
        const PRUnichar *formatStrings[2] = {fileStr.get(), typeStr.get()};
        NS_ConvertASCIItoUTF16 fmtName(aFormatNames[eWithFile]);
        mStringBundle->FormatStringFromName(fmtName.get(), formatStrings, 2,
                                            getter_Copies(title));
      }
      else {
        const PRUnichar *formatStrings[1] = {typeStr.get()};
        NS_ConvertASCIItoUTF16 fmtName(aFormatNames[eWithNoInfo]);
        mStringBundle->FormatStringFromName(fmtName.get(), formatStrings, 1,
                                            getter_Copies(title));
      }
    }
  } 

  
  if (aStatus.IsEmpty()) {
    SetTitle(title);
  }
  else {
    nsXPIDLString titleWithStatus;
    const nsPromiseFlatString& status = PromiseFlatString(aStatus);
    const PRUnichar *formatStrings[2] = {title.get(), status.get()};
    NS_NAMED_LITERAL_STRING(fmtName, "TitleWithStatus");
    mStringBundle->FormatStringFromName(fmtName.get(), formatStrings, 2,
                                        getter_Copies(titleWithStatus));
    SetTitle(titleWithStatus);
  }
}
