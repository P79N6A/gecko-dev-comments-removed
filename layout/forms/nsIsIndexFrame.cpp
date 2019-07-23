




































#include "nsIsIndexFrame.h"

#include "nsIContent.h"
#include "prtypes.h"
#include "nsIAtom.h"
#include "nsPresContext.h"
#include "nsGkAtoms.h"
#include "nsPresState.h"
#include "nsWidgetsCID.h"
#include "nsIComponentManager.h"
#include "nsHTMLParts.h"
#include "nsIDOMHTMLInputElement.h"
#include "nsINameSpaceManager.h"
#include "nsCOMPtr.h"
#include "nsIDOMElement.h"
#include "nsIDOMDocument.h"
#include "nsIDocument.h"
#include "nsIPresShell.h"
#include "nsIDOMHTMLInputElement.h"
#include "nsIStatefulFrame.h"
#include "nsXPCOM.h"
#include "nsISupportsPrimitives.h"
#include "nsIComponentManager.h"
#include "nsHTMLParts.h"
#include "nsLinebreakConverter.h"
#include "nsILinkHandler.h"
#include "nsIHTMLDocument.h"
#include "nsXPIDLString.h"
#include "nsReadableUtils.h"
#include "nsNetUtil.h"
#include "nsICharsetConverterManager.h"
#include "nsEscape.h"
#include "nsIDOMKeyListener.h"
#include "nsIDOMKeyEvent.h"
#include "nsIFormControlFrame.h"
#include "nsINodeInfo.h"
#include "nsIDOMEventReceiver.h"
#include "nsContentCID.h"
#include "nsNodeInfoManager.h"
#include "nsContentCreatorFunctions.h"
#include "nsContentUtils.h"

nsIFrame*
NS_NewIsIndexFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsIsIndexFrame(aContext);
}

nsIsIndexFrame::nsIsIndexFrame(nsStyleContext* aContext) :
  nsAreaFrame(aContext)
{
  SetFlags(NS_BLOCK_SPACE_MGR);
}

nsIsIndexFrame::~nsIsIndexFrame()
{
}

void
nsIsIndexFrame::Destroy()
{
  
  if (mInputContent) {
    nsCOMPtr<nsIDOMEventReceiver> receiver(do_QueryInterface(mInputContent));
    receiver->RemoveEventListenerByIID(this, NS_GET_IID(nsIDOMKeyListener));
    nsContentUtils::DestroyAnonymousContent(&mInputContent);
  }
  nsContentUtils::DestroyAnonymousContent(&mTextContent);
  nsContentUtils::DestroyAnonymousContent(&mPreHr);
  nsContentUtils::DestroyAnonymousContent(&mPostHr);
  nsAreaFrame::Destroy();
}




NS_IMETHODIMP
nsIsIndexFrame::UpdatePromptLabel()
{
  if (!mTextContent) return NS_ERROR_UNEXPECTED;

  nsresult result = NS_OK;

  
  
  nsXPIDLString prompt;
  if (mContent)
    mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::prompt, prompt);

  if (prompt.IsEmpty()) {
    
    
    
    
    result =
      nsContentUtils::GetLocalizedString(nsContentUtils::eFORMS_PROPERTIES,
                                         "IsIndexPrompt", prompt);
  }

  mTextContent->SetText(prompt, PR_TRUE);

  return NS_OK;
}

nsresult
nsIsIndexFrame::GetInputFrame(nsIFormControlFrame** oFrame)
{
  nsIPresShell *presShell = GetPresContext()->GetPresShell();
  if (!mInputContent) NS_WARNING("null content - cannot restore state");
  if (presShell && mInputContent) {
    nsIFrame *frame = presShell->GetPrimaryFrameFor(mInputContent);
    if (frame) {
      return CallQueryInterface(frame, oFrame);
    }
  }
  return NS_OK;
}

void
nsIsIndexFrame::GetInputValue(nsString& oString)
{
  nsIFormControlFrame* frame = nsnull;
  GetInputFrame(&frame);
  if (frame) {
    ((nsNewFrame*)frame)->GetValue(oString, PR_FALSE);
  }
}

void
nsIsIndexFrame::SetInputValue(const nsString& aString)
{
  nsIFormControlFrame* frame = nsnull;
  GetInputFrame(&frame);
  if (frame) {
    ((nsNewFrame*)frame)->SetValue(aString);
  }
}

void 
nsIsIndexFrame::SetFocus(PRBool aOn, PRBool aRepaint)
{
  nsIFormControlFrame* frame = nsnull;
  GetInputFrame(&frame);
  if (frame) {
    frame->SetFocus(aOn, aRepaint);
  }
}

nsresult
nsIsIndexFrame::CreateAnonymousContent(nsTArray<nsIContent*>& aElements)
{
  
  nsCOMPtr<nsIDocument> doc = mContent->GetDocument();
  nsNodeInfoManager *nimgr = doc->NodeInfoManager();

  
  nsCOMPtr<nsINodeInfo> hrInfo;
  nimgr->GetNodeInfo(nsGkAtoms::hr, nsnull, kNameSpaceID_None,
                     getter_AddRefs(hrInfo));

  NS_NewHTMLElement(getter_AddRefs(mPreHr), hrInfo);
  if (!mPreHr || !aElements.AppendElement(mPreHr))
    return NS_ERROR_OUT_OF_MEMORY;

  
  NS_NewTextNode(getter_AddRefs(mTextContent), nimgr);
  if (!mTextContent)
    return NS_ERROR_OUT_OF_MEMORY;

  
  UpdatePromptLabel();
  if (!aElements.AppendElement(mTextContent))
    return NS_ERROR_OUT_OF_MEMORY;

  
  nsCOMPtr<nsINodeInfo> inputInfo;
  nimgr->GetNodeInfo(nsGkAtoms::input, nsnull, kNameSpaceID_None,
                     getter_AddRefs(inputInfo));

  NS_NewHTMLElement(getter_AddRefs(mInputContent), inputInfo);
  if (!mInputContent)
    return NS_ERROR_OUT_OF_MEMORY;

  mInputContent->SetAttr(kNameSpaceID_None, nsGkAtoms::type,
                         NS_LITERAL_STRING("text"), PR_FALSE);

  if (!aElements.AppendElement(mInputContent))
    return NS_ERROR_OUT_OF_MEMORY;

  
  nsCOMPtr<nsIDOMEventReceiver> receiver(do_QueryInterface(mInputContent));
  receiver->AddEventListenerByIID(this, NS_GET_IID(nsIDOMKeyListener));

  
  NS_NewHTMLElement(getter_AddRefs(mPostHr), hrInfo);
  if (!mPostHr || !aElements.AppendElement(mPostHr))
    return NS_ERROR_OUT_OF_MEMORY;

  return NS_OK;
}


NS_IMETHODIMP
nsIsIndexFrame::QueryInterface(const nsIID& aIID, void** aInstancePtr)
{
  NS_PRECONDITION(aInstancePtr, "null ptr");
  if (NS_UNLIKELY(!aInstancePtr)) {
    return NS_ERROR_NULL_POINTER;
  }
  if (aIID.Equals(NS_GET_IID(nsIAnonymousContentCreator))) {
    *aInstancePtr = NS_STATIC_CAST(nsIAnonymousContentCreator*, this);
    return NS_OK;
  }
  if (aIID.Equals(NS_GET_IID(nsIStatefulFrame))) {
    *aInstancePtr = NS_STATIC_CAST(nsIStatefulFrame*, this);
    return NS_OK;
  }
  if (aIID.Equals(NS_GET_IID(nsIDOMKeyListener))) {
    *aInstancePtr = NS_STATIC_CAST(nsIDOMKeyListener*, this);
    return NS_OK;
  }
  return nsAreaFrame::QueryInterface(aIID, aInstancePtr);
}

nscoord
nsIsIndexFrame::GetMinWidth(nsIRenderingContext *aRenderingContext)
{
  nscoord result;
  DISPLAY_MIN_WIDTH(this, result);

  
  
  result = GetPrefWidth(aRenderingContext);
  return result;
}

PRBool
nsIsIndexFrame::IsLeaf() const
{
  return PR_TRUE;
}

NS_IMETHODIMP
nsIsIndexFrame::AttributeChanged(PRInt32         aNameSpaceID,
                                 nsIAtom*        aAttribute,
                                 PRInt32         aModType)
{
  nsresult rv = NS_OK;
  if (nsGkAtoms::prompt == aAttribute) {
    rv = UpdatePromptLabel();
  } else {
    rv = nsAreaFrame::AttributeChanged(aNameSpaceID, aAttribute, aModType);
  }
  return rv;
}


nsresult 
nsIsIndexFrame::KeyPress(nsIDOMEvent* aEvent)
{
  nsCOMPtr<nsIDOMKeyEvent> keyEvent = do_QueryInterface(aEvent);
  if (keyEvent) {
    PRUint32 code;
    keyEvent->GetKeyCode(&code);
    if (code == 0) {
      keyEvent->GetCharCode(&code);
    }
    if (nsIDOMKeyEvent::DOM_VK_RETURN == code) {
      OnSubmit(GetPresContext());
      aEvent->PreventDefault(); 
    }
  }

  return NS_OK;
}

#ifdef NS_DEBUG
NS_IMETHODIMP
nsIsIndexFrame::GetFrameName(nsAString& aResult) const
{
  return MakeFrameName(NS_LITERAL_STRING("IsIndex"), aResult);
}
#endif



NS_IMETHODIMP
nsIsIndexFrame::OnSubmit(nsPresContext* aPresContext)
{
  if (!mContent || !mInputContent) {
    return NS_ERROR_UNEXPECTED;
  }

  nsresult result = NS_OK;

  
  nsAutoString data;

  nsCOMPtr<nsIUnicodeEncoder> encoder;
  if(NS_FAILED(GetEncoder(getter_AddRefs(encoder))))  
     encoder = nsnull;

  nsAutoString value;
  GetInputValue(value);
  URLEncode(value, encoder, data);
  

  
  nsILinkHandler *handler = aPresContext->GetLinkHandler();

  nsAutoString href;

  
  
  
  nsCOMPtr<nsIDocument> document = mContent->GetDocument();
  if (!document) return NS_OK; 

  
  nsIURI *baseURI = document->GetBaseURI();
  if (!baseURI) {
    NS_ERROR("No Base URL found in Form Submit!\n");
    return NS_OK; 
  }

  
  
  
  
  
  

  nsresult rv;
  nsCOMPtr<nsIHTMLDocument> htmlDoc;
  htmlDoc = do_QueryInterface(document, &rv);
  if (NS_FAILED(rv)) {   
    
    
    return NS_OK;
  } 

  
  
  nsCAutoString relPath;
  baseURI->GetSpec(relPath);
  if (!relPath.IsEmpty()) {
    CopyUTF8toUTF16(relPath, href);

    
    PRInt32 queryStart = href.FindChar('?');
    if (kNotFound != queryStart) {
      href.Truncate(queryStart);
    }
  } else {
    NS_ERROR("Rel path couldn't be formed in form submit!\n");
    return NS_ERROR_OUT_OF_MEMORY;
  }

  
  
  nsCOMPtr<nsIURI> actionURL;
  nsXPIDLCString scheme;
  PRBool isJSURL = PR_FALSE;
  const nsACString &docCharset = document->GetDocumentCharacterSet();
  const nsPromiseFlatCString& flatDocCharset = PromiseFlatCString(docCharset);

  if (NS_SUCCEEDED(result = NS_NewURI(getter_AddRefs(actionURL), href,
                                      flatDocCharset.get(),
                                      baseURI))) {
    result = actionURL->SchemeIs("javascript", &isJSURL);
  }
  
  if (!isJSURL) { 
    if (href.FindChar('?') == kNotFound) { 
      href.Append(PRUnichar('?'));
    } else {                              
      if (href.Last() != '&' && href.Last() != '?') {   
        href.Append(PRUnichar('&'));
      }
    }
    href.Append(data);
  }
  nsCOMPtr<nsIURI> uri;
  result = NS_NewURI(getter_AddRefs(uri), href,
                     flatDocCharset.get(), baseURI);
  if (NS_FAILED(result)) return result;

  
  if (handler) {
    handler->OnLinkClick(mContent, uri, nsnull);
  }
  return result;
}

void nsIsIndexFrame::GetSubmitCharset(nsCString& oCharset)
{
  oCharset.AssignLiteral("UTF-8"); 
  
  
  

  
  nsIDocument* doc = mContent->GetDocument();
  if (doc) {
    oCharset = doc->GetDocumentCharacterSet();
  }
}

NS_IMETHODIMP nsIsIndexFrame::GetEncoder(nsIUnicodeEncoder** encoder)
{
  *encoder = nsnull;
  nsCAutoString charset;
  nsresult rv = NS_OK;
  GetSubmitCharset(charset);
  
  
  nsICharsetConverterManager * ccm = nsnull;
  rv = CallGetService(NS_CHARSETCONVERTERMANAGER_CONTRACTID, &ccm);
  if(NS_SUCCEEDED(rv) && (nsnull != ccm)) {
     rv = ccm->GetUnicodeEncoderRaw(charset.get(), encoder);
     NS_RELEASE(ccm);
     if (!*encoder) {
       rv = NS_ERROR_FAILURE;
     }
     if (NS_SUCCEEDED(rv)) {
       rv = (*encoder)->SetOutputErrorBehavior(nsIUnicodeEncoder::kOnError_Replace, nsnull, (PRUnichar)'?');
     }
  }
  return rv;
}


char*
nsIsIndexFrame::UnicodeToNewBytes(const PRUnichar* aSrc, PRUint32 aLen, nsIUnicodeEncoder* encoder)
{
   char* res = nsnull;
   if(NS_SUCCEEDED(encoder->Reset()))
   {
      PRInt32 maxByteLen = 0;
      if(NS_SUCCEEDED(encoder->GetMaxLength(aSrc, (PRInt32) aLen, &maxByteLen))) 
      {
          res = new char[maxByteLen+1];
          if(nsnull != res) 
          {
             PRInt32 reslen = maxByteLen;
             PRInt32 reslen2 ;
             PRInt32 srclen = aLen;
             encoder->Convert(aSrc, &srclen, res, &reslen);
             reslen2 = maxByteLen-reslen;
             encoder->Finish(res+reslen, &reslen2);
             res[reslen+reslen2] = '\0';
          }
      }

   }
   return res;
}


void
nsIsIndexFrame::URLEncode(const nsString& aString, nsIUnicodeEncoder* encoder, nsString& oString) 
{
  char* inBuf = nsnull;
  if(encoder)
    inBuf  = UnicodeToNewBytes(aString.get(), aString.Length(), encoder);

  if(nsnull == inBuf)
    inBuf  = ToNewCString(aString);

  
  char* convertedBuf = nsLinebreakConverter::ConvertLineBreaks(inBuf,
                           nsLinebreakConverter::eLinebreakAny, nsLinebreakConverter::eLinebreakNet);
  delete [] inBuf;
  
  char* outBuf = nsEscape(convertedBuf, url_XPAlphas);
  oString.AssignASCII(outBuf);
  nsMemory::Free(outBuf);
  nsMemory::Free(convertedBuf);
}




NS_IMETHODIMP
nsIsIndexFrame::SaveState(SpecialStateID aStateID, nsPresState** aState)
{
  NS_ENSURE_ARG_POINTER(aState);

  
  nsAutoString stateString;
  GetInputValue(stateString);

  nsresult res = NS_OK;
  if (! stateString.IsEmpty()) {

    
    res = NS_NewPresState(aState);
    NS_ENSURE_SUCCESS(res, res);
    res = (*aState)->SetStateProperty(NS_LITERAL_STRING("value"), stateString);
  }

  return res;
}

NS_IMETHODIMP
nsIsIndexFrame::RestoreState(nsPresState* aState)
{
  NS_ENSURE_ARG_POINTER(aState);

  
  nsAutoString stateString;
  nsresult res = aState->GetStateProperty(NS_LITERAL_STRING("value"), stateString);
  NS_ENSURE_SUCCESS(res, res);

  SetInputValue(stateString);
  return NS_OK;
}
