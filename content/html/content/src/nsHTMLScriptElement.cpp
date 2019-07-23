




































#include "nsIDOMHTMLScriptElement.h"
#include "nsIDOMNSHTMLScriptElement.h"
#include "nsIDOMEventTarget.h"
#include "nsGenericHTMLElement.h"
#include "nsGkAtoms.h"
#include "nsStyleConsts.h"
#include "nsIDocument.h"
#include "nsScriptElement.h"
#include "nsIURI.h"
#include "nsNetUtil.h"

#include "nsUnicharUtils.h"  
#include "jsapi.h"
#include "nsIScriptContext.h"
#include "nsIScriptGlobalObject.h"
#include "nsIXPConnect.h"
#include "nsServiceManagerUtils.h"
#include "nsIScriptEventHandler.h"
#include "nsIDOMDocument.h"
#include "nsContentErrors.h"
#include "nsIArray.h"
#include "nsTArray.h"
#include "nsDOMJSUtils.h"





class nsHTMLScriptEventHandler : public nsIScriptEventHandler
{
public:
  nsHTMLScriptEventHandler(nsIDOMHTMLScriptElement *aOuter);
  virtual ~nsHTMLScriptEventHandler() {}

  
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSISCRIPTEVENTHANDLER

  
  nsresult ParseEventString(const nsAString &aValue);

protected:
  
  nsIDOMHTMLScriptElement *mOuter;

  
  nsTArray<nsCString> mArgNames;

  
  nsString mEventName;
};


nsHTMLScriptEventHandler::nsHTMLScriptEventHandler(nsIDOMHTMLScriptElement *aOuter)
{

  
  mOuter = aOuter;
}




NS_IMPL_ADDREF (nsHTMLScriptEventHandler)
NS_IMPL_RELEASE(nsHTMLScriptEventHandler)

NS_INTERFACE_MAP_BEGIN(nsHTMLScriptEventHandler)

NS_INTERFACE_MAP_END_AGGREGATED(mOuter)





nsresult nsHTMLScriptEventHandler::ParseEventString(const nsAString &aValue)
{
  nsAutoString eventSig(aValue);
  nsAutoString::const_iterator start, next, end;

  
  mArgNames.Clear();

  
  eventSig.StripWhitespace();

  
  eventSig.BeginReading(start);
  eventSig.EndReading(end);

  next = start;
  if (FindCharInReadable('(', next, end)) {
    mEventName = Substring(start, next);
  } else {
    
    return NS_ERROR_FAILURE;
  }

  ++next;  
  --end;   
  if (*end != ')') {
    
    return NS_ERROR_FAILURE;
  }

  
  NS_LossyConvertUTF16toASCII sig(Substring(next, end));

  
  ParseString(sig, ',', mArgNames);

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLScriptEventHandler::IsSameEvent(const nsAString &aObjectName,
                                      const nsAString &aEventName,
                                      PRUint32 aArgCount,
                                      PRBool *aResult)
{
  *aResult = PR_FALSE;

  
  
  
  if (aEventName.Equals(mEventName, nsCaseInsensitiveStringComparator())) {
    nsAutoString id;

    
    mOuter->GetHtmlFor(id);
    if (aObjectName.Equals(id)) {
      *aResult = PR_TRUE;
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLScriptEventHandler::Invoke(nsISupports *aTargetObject,
                                 void *aArgs,
                                 PRUint32 aArgCount)
{
  nsresult rv;
  nsAutoString scriptBody;

  
  
  
  
  
  if (!aTargetObject || (aArgCount && !aArgs) ) {
    return NS_ERROR_FAILURE;
  }

  
  rv = mOuter->GetText(scriptBody);
  if (NS_FAILED(rv)) {
    return rv;
  }

  
  PRUint32 lineNumber = 0;
  nsCOMPtr<nsIScriptElement> sele(do_QueryInterface(mOuter));
  if (sele) {
    lineNumber = sele->GetScriptLineNumber();
  }

  
  nsCOMPtr<nsIDOMDocument> domdoc;
  nsCOMPtr<nsIScriptContext> scriptContext;
  nsIScriptGlobalObject *sgo = nsnull;

  mOuter->GetOwnerDocument(getter_AddRefs(domdoc));

  nsCOMPtr<nsIDocument> doc(do_QueryInterface(domdoc));
  if (doc) {
    sgo = doc->GetScriptGlobalObject();
    if (sgo) {
      scriptContext = sgo->GetContext();
    }
  }
  
  if (!scriptContext) {
    return NS_ERROR_FAILURE;
  }

  
  JSContext *cx = (JSContext *)scriptContext->GetNativeContext();
  JSObject *scope = sgo->GetGlobalJSObject();

  nsCOMPtr<nsIXPConnectJSObjectHolder> holder;
  jsval v;
  rv = nsContentUtils::WrapNative(cx, scope, aTargetObject, &v,
                                  getter_AddRefs(holder));
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  
  
  
  
  
  
  const int kMaxArgsOnStack = 10;

  PRInt32 argc, i;
  const char** args;
  const char*  stackArgs[kMaxArgsOnStack];

  args = stackArgs;
  argc = PRInt32(mArgNames.Length());

  
  
  if (argc >= kMaxArgsOnStack) {
    args = new const char*[argc+1];
    if (!args) return NS_ERROR_OUT_OF_MEMORY;
  }

  for(i=0; i<argc; i++) {
    args[i] = mArgNames[i].get();
  }

  
  args[i] = nsnull;

  
  void* funcObject = nsnull;
  NS_NAMED_LITERAL_CSTRING(funcName, "anonymous");

  rv = scriptContext->CompileFunction(JSVAL_TO_OBJECT(v),
                                      funcName,   
                                      argc,       
                                      args,       
                                      scriptBody, 
                                      nsnull,     
                                      lineNumber, 
                                      JSVERSION_DEFAULT, 
                                      PR_FALSE,   
                                      &funcObject);
  
  if (args != stackArgs) {
    delete [] args;
  }

  
  if (NS_FAILED(rv)) {
    return rv;
  }

  
  
  nsCOMPtr<nsIArray> argarray;
  rv = NS_CreateJSArgv(cx, aArgCount, (jsval *)aArgs, getter_AddRefs(argarray));
  if (NS_FAILED(rv))
    return rv;

  
  nsCOMPtr<nsIVariant> ret;
  return scriptContext->CallEventHandler(aTargetObject, scope, funcObject,
                                         argarray, getter_AddRefs(ret));
}


class nsHTMLScriptElement : public nsGenericHTMLElement,
                            public nsIDOMHTMLScriptElement,
                            public nsIDOMNSHTMLScriptElement,
                            public nsScriptElement
{
public:
  nsHTMLScriptElement(nsINodeInfo *aNodeInfo, PRBool aFromParser);
  virtual ~nsHTMLScriptElement();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE(nsGenericHTMLElement::)

  
  NS_FORWARD_NSIDOMELEMENT(nsGenericHTMLElement::)

  
  NS_FORWARD_NSIDOMHTMLELEMENT(nsGenericHTMLElement::)

  NS_DECL_NSIDOMHTMLSCRIPTELEMENT
  NS_DECL_NSIDOMNSHTMLSCRIPTELEMENT

  
  virtual void GetScriptType(nsAString& type);
  virtual already_AddRefed<nsIURI> GetScriptURI();
  virtual void GetScriptText(nsAString& text);
  virtual void GetScriptCharset(nsAString& charset);
  virtual PRBool GetScriptDeferred();
  virtual PRBool GetScriptAsync();

  
  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              PRBool aCompileEventHandlers);

  virtual nsresult GetInnerHTML(nsAString& aInnerHTML);
  virtual nsresult SetInnerHTML(const nsAString& aInnerHTML);
  virtual nsresult DoneAddingChildren(PRBool aHaveNotified);
  virtual PRBool IsDoneAddingChildren();

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

protected:
  PRBool IsOnloadEventForWindow();


  
  nsCOMPtr<nsHTMLScriptEventHandler> mScriptEventHandler;

  
  virtual PRBool HasScriptContent();
  virtual nsresult MaybeProcessScript();
};


NS_IMPL_NS_NEW_HTML_ELEMENT_CHECK_PARSER(Script)


nsHTMLScriptElement::nsHTMLScriptElement(nsINodeInfo *aNodeInfo,
                                         PRBool aFromParser)
  : nsGenericHTMLElement(aNodeInfo)
{
  mDoneAddingChildren = !aFromParser;
  AddMutationObserver(this);
}

nsHTMLScriptElement::~nsHTMLScriptElement()
{
}


NS_IMPL_ADDREF_INHERITED(nsHTMLScriptElement, nsGenericElement)
NS_IMPL_RELEASE_INHERITED(nsHTMLScriptElement, nsGenericElement)


NS_INTERFACE_TABLE_HEAD(nsHTMLScriptElement)
  NS_HTML_CONTENT_INTERFACE_TABLE5(nsHTMLScriptElement,
                                   nsIDOMHTMLScriptElement,
                                   nsIScriptLoaderObserver,
                                   nsIScriptElement,
                                   nsIDOMNSHTMLScriptElement,
                                   nsIMutationObserver)
  NS_HTML_CONTENT_INTERFACE_TABLE_TO_MAP_SEGUE(nsHTMLScriptElement,
                                               nsGenericHTMLElement)
  if (mScriptEventHandler && aIID.Equals(NS_GET_IID(nsIScriptEventHandler)))
    foundInterface = static_cast<nsIScriptEventHandler*>
                                (mScriptEventHandler);
  else
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(HTMLScriptElement)
NS_HTML_CONTENT_INTERFACE_MAP_END


nsresult
nsHTMLScriptElement::BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                                nsIContent* aBindingParent,
                                PRBool aCompileEventHandlers)
{
  nsresult rv = nsGenericHTMLElement::BindToTree(aDocument, aParent,
                                                 aBindingParent,
                                                 aCompileEventHandlers);
  NS_ENSURE_SUCCESS(rv, rv);

  if (aDocument) {
    MaybeProcessScript();
  }

  return NS_OK;
}

nsresult
nsHTMLScriptElement::Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const
{
  *aResult = nsnull;

  nsHTMLScriptElement* it = new nsHTMLScriptElement(aNodeInfo, PR_FALSE);
  if (!it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  nsCOMPtr<nsINode> kungFuDeathGrip = it;
  nsresult rv = CopyInnerTo(it);
  NS_ENSURE_SUCCESS(rv, rv);

  
  it->mIsEvaluated = mIsEvaluated;
  it->mLineNumber = mLineNumber;
  it->mMalformed = mMalformed;

  kungFuDeathGrip.swap(*aResult);

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLScriptElement::GetText(nsAString& aValue)
{
  nsContentUtils::GetNodeTextContent(this, PR_FALSE, aValue);
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLScriptElement::SetText(const nsAString& aValue)
{
  return nsContentUtils::SetNodeTextContent(this, aValue, PR_TRUE);
}


NS_IMPL_STRING_ATTR(nsHTMLScriptElement, Charset, charset)
NS_IMPL_BOOL_ATTR(nsHTMLScriptElement, Defer, defer)
NS_IMPL_BOOL_ATTR(nsHTMLScriptElement, Async, async)
NS_IMPL_URI_ATTR(nsHTMLScriptElement, Src, src)
NS_IMPL_STRING_ATTR(nsHTMLScriptElement, Type, type)
NS_IMPL_STRING_ATTR(nsHTMLScriptElement, HtmlFor, _for)
NS_IMPL_STRING_ATTR(nsHTMLScriptElement, Event, event)

nsresult
nsHTMLScriptElement::GetInnerHTML(nsAString& aInnerHTML)
{
  nsContentUtils::GetNodeTextContent(this, PR_FALSE, aInnerHTML);
  return NS_OK;
}

nsresult
nsHTMLScriptElement::SetInnerHTML(const nsAString& aInnerHTML)
{
  return nsContentUtils::SetNodeTextContent(this, aInnerHTML, PR_TRUE);
}

nsresult
nsHTMLScriptElement::DoneAddingChildren(PRBool aHaveNotified)
{
  mDoneAddingChildren = PR_TRUE;
  return MaybeProcessScript();
}

PRBool
nsHTMLScriptElement::IsDoneAddingChildren()
{
  return mDoneAddingChildren;
}




void
nsHTMLScriptElement::GetScriptType(nsAString& type)
{
  GetType(type);
}




already_AddRefed<nsIURI>
nsHTMLScriptElement::GetScriptURI()
{
  nsIURI *uri = nsnull;
  nsAutoString src;
  GetSrc(src);
  if (!src.IsEmpty())
    NS_NewURI(&uri, src);
  return uri;
}

void
nsHTMLScriptElement::GetScriptText(nsAString& text)
{
  GetText(text);
}

void
nsHTMLScriptElement::GetScriptCharset(nsAString& charset)
{
  GetCharset(charset);
}

PRBool
nsHTMLScriptElement::GetScriptDeferred()
{
  PRBool defer, async;
  GetAsync(&async);
  GetDefer(&defer);
  nsCOMPtr<nsIURI> uri = GetScriptURI();

  return !async && defer && uri;
}

PRBool
nsHTMLScriptElement::GetScriptAsync()
{
  PRBool async;
  GetAsync(&async);
  nsCOMPtr<nsIURI> uri = GetScriptURI();

  return async && uri;
}

PRBool
nsHTMLScriptElement::HasScriptContent()
{
  return HasAttr(kNameSpaceID_None, nsGkAtoms::src) ||
         nsContentUtils::HasNonEmptyTextContent(this);
}

nsresult
nsHTMLScriptElement::MaybeProcessScript()
{
  nsresult rv = nsScriptElement::MaybeProcessScript();
  if (rv == NS_CONTENT_SCRIPT_IS_EVENTHANDLER) {
    
    rv = NS_OK;

    
    
    NS_ASSERTION(mIsEvaluated, "should have set mIsEvaluated already");
    NS_ASSERTION(!mScriptEventHandler, "how could we have an SEH already?");

    mScriptEventHandler = new nsHTMLScriptEventHandler(this);
    NS_ENSURE_TRUE(mScriptEventHandler, NS_ERROR_OUT_OF_MEMORY);

    nsAutoString event_val;
    GetAttr(kNameSpaceID_None, nsGkAtoms::event, event_val);
    mScriptEventHandler->ParseEventString(event_val);
  }

  return rv;
}
