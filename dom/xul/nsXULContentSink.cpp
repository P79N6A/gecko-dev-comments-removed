













#include "nsXULContentSink.h"

#include "jsfriendapi.h"

#include "nsCOMPtr.h"
#include "nsForwardReference.h"
#include "nsHTMLStyleSheet.h"
#include "nsIContentSink.h"
#include "nsIDocument.h"
#include "nsIDOMEventListener.h"
#include "nsIDOMHTMLFormElement.h"
#include "nsIDOMXULDocument.h"
#include "nsIFormControl.h"
#include "nsIProgrammingLanguage.h"
#include "mozilla/dom/NodeInfo.h"
#include "nsIScriptContext.h"
#include "nsIScriptGlobalObject.h"
#include "nsIServiceManager.h"
#include "nsIURL.h"
#include "nsNameSpaceManager.h"
#include "nsParserBase.h"
#include "nsViewManager.h"
#include "nsIXULDocument.h"
#include "nsIScriptSecurityManager.h"
#include "nsLayoutCID.h"
#include "nsNetUtil.h"
#include "nsRDFCID.h"
#include "nsXPIDLString.h"
#include "nsReadableUtils.h"
#include "nsXULElement.h"
#include "prlog.h"
#include "prmem.h"
#include "nsCRT.h"

#include "nsXULPrototypeDocument.h"     
#include "mozilla/css/Loader.h"

#include "nsUnicharUtils.h"
#include "nsGkAtoms.h"
#include "nsContentUtils.h"
#include "nsAttrName.h"
#include "nsXMLContentSink.h"
#include "nsIConsoleService.h"
#include "nsIScriptError.h"
#include "nsContentTypeParser.h"

#ifdef PR_LOGGING
static PRLogModuleInfo* gContentSinkLog;
#endif



XULContentSinkImpl::ContextStack::ContextStack()
    : mTop(nullptr), mDepth(0)
{
}

XULContentSinkImpl::ContextStack::~ContextStack()
{
    while (mTop) {
        Entry* doomed = mTop;
        mTop = mTop->mNext;
        delete doomed;
    }
}

nsresult
XULContentSinkImpl::ContextStack::Push(nsXULPrototypeNode* aNode, State aState)
{
    Entry* entry = new Entry;
    if (! entry)
        return NS_ERROR_OUT_OF_MEMORY;

    entry->mNode  = aNode;
    entry->mState = aState;
    entry->mNext  = mTop;
    mTop = entry;

    ++mDepth;
    return NS_OK;
}

nsresult
XULContentSinkImpl::ContextStack::Pop(State* aState)
{
    if (mDepth == 0)
        return NS_ERROR_UNEXPECTED;

    Entry* entry = mTop;
    mTop = mTop->mNext;
    --mDepth;

    *aState = entry->mState;
    delete entry;

    return NS_OK;
}


nsresult
XULContentSinkImpl::ContextStack::GetTopNode(nsRefPtr<nsXULPrototypeNode>& aNode)
{
    if (mDepth == 0)
        return NS_ERROR_UNEXPECTED;

    aNode = mTop->mNode;
    return NS_OK;
}


nsresult
XULContentSinkImpl::ContextStack::GetTopChildren(nsPrototypeArray** aChildren)
{
    if (mDepth == 0)
        return NS_ERROR_UNEXPECTED;

    *aChildren = &(mTop->mChildren);
    return NS_OK;
}

void
XULContentSinkImpl::ContextStack::Clear()
{
  Entry *cur = mTop;
  while (cur) {
    
    Entry *next = cur->mNext;
    delete cur;
    cur = next;
  }

  mTop = nullptr;
  mDepth = 0;
}

void
XULContentSinkImpl::ContextStack::Traverse(nsCycleCollectionTraversalCallback& aCb)
{
  nsCycleCollectionTraversalCallback& cb = aCb;
  for (ContextStack::Entry* tmp = mTop; tmp; tmp = tmp->mNext) {
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mNode)
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mChildren)
  }
}




XULContentSinkImpl::XULContentSinkImpl()
    : mText(nullptr),
      mTextLength(0),
      mTextSize(0),
      mConstrainSize(true),
      mState(eInProlog),
      mParser(nullptr)
{

#ifdef PR_LOGGING
    if (! gContentSinkLog)
        gContentSinkLog = PR_NewLogModule("nsXULContentSink");
#endif
}


XULContentSinkImpl::~XULContentSinkImpl()
{
    NS_IF_RELEASE(mParser); 

    
    NS_ASSERTION(mContextStack.Depth() == 0, "Context stack not empty?");
    mContextStack.Clear();

    free(mText);
}




NS_IMPL_CYCLE_COLLECTION_CLASS(XULContentSinkImpl)

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(XULContentSinkImpl)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mNodeInfoManager)
  tmp->mContextStack.Clear();
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mPrototype)
  NS_IF_RELEASE(tmp->mParser);
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(XULContentSinkImpl)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mNodeInfoManager)
  tmp->mContextStack.Traverse(cb);
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mPrototype)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_RAWPTR(mParser)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(XULContentSinkImpl)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIXMLContentSink)
  NS_INTERFACE_MAP_ENTRY(nsIXMLContentSink)
  NS_INTERFACE_MAP_ENTRY(nsIExpatSink)
  NS_INTERFACE_MAP_ENTRY(nsIContentSink)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(XULContentSinkImpl)
NS_IMPL_CYCLE_COLLECTING_RELEASE(XULContentSinkImpl)




NS_IMETHODIMP 
XULContentSinkImpl::WillBuildModel(nsDTDMode aDTDMode)
{
#if FIXME
    if (! mParentContentSink) {
        
        
        mDocument->BeginLoad();
    }
#endif

    return NS_OK;
}

NS_IMETHODIMP 
XULContentSinkImpl::DidBuildModel(bool aTerminated)
{
    nsCOMPtr<nsIDocument> doc = do_QueryReferent(mDocument);
    if (doc) {
        doc->EndLoad();
        mDocument = nullptr;
    }

    
    
    NS_IF_RELEASE(mParser);
    return NS_OK;
}

NS_IMETHODIMP 
XULContentSinkImpl::WillInterrupt(void)
{
    
    return NS_OK;
}

NS_IMETHODIMP 
XULContentSinkImpl::WillResume(void)
{
    
    return NS_OK;
}

NS_IMETHODIMP 
XULContentSinkImpl::SetParser(nsParserBase* aParser)
{
    NS_IF_RELEASE(mParser);
    mParser = aParser;
    NS_IF_ADDREF(mParser);
    return NS_OK;
}

NS_IMETHODIMP 
XULContentSinkImpl::SetDocumentCharset(nsACString& aCharset)
{
    nsCOMPtr<nsIDocument> doc = do_QueryReferent(mDocument);
    if (doc) {
        doc->SetDocumentCharacterSet(aCharset);
    }
  
    return NS_OK;
}

nsISupports *
XULContentSinkImpl::GetTarget()
{
    nsCOMPtr<nsIDocument> doc = do_QueryReferent(mDocument);
    return doc;    
}



nsresult
XULContentSinkImpl::Init(nsIDocument* aDocument,
                         nsXULPrototypeDocument* aPrototype)
{
    NS_PRECONDITION(aDocument != nullptr, "null ptr");
    if (! aDocument)
        return NS_ERROR_NULL_POINTER;
    
    nsresult rv;

    mDocument    = do_GetWeakReference(aDocument);
    mPrototype   = aPrototype;

    mDocumentURL = mPrototype->GetURI();

    
    
    
    nsAutoString preferredStyle;
    rv = mPrototype->GetHeaderData(nsGkAtoms::headerDefaultStyle,
                                   preferredStyle);
    if (NS_FAILED(rv)) return rv;

    if (!preferredStyle.IsEmpty()) {
        aDocument->SetHeaderData(nsGkAtoms::headerDefaultStyle,
                                 preferredStyle);
    }

    
    aDocument->CSSLoader()->SetPreferredSheet(preferredStyle);

    mNodeInfoManager = aPrototype->GetNodeInfoManager();
    if (! mNodeInfoManager)
        return NS_ERROR_UNEXPECTED;

    mState = eInProlog;
    return NS_OK;
}







bool
XULContentSinkImpl::IsDataInBuffer(char16_t* buffer, int32_t length)
{
    for (int32_t i = 0; i < length; ++i) {
        if (buffer[i] == ' ' ||
            buffer[i] == '\t' ||
            buffer[i] == '\n' ||
            buffer[i] == '\r')
            continue;

        return true;
    }
    return false;
}


nsresult
XULContentSinkImpl::FlushText(bool aCreateTextNode)
{
    nsresult rv;

    do {
        
        
        if (! mTextLength)
            break;

        if (! aCreateTextNode)
            break;

        nsRefPtr<nsXULPrototypeNode> node;
        rv = mContextStack.GetTopNode(node);
        if (NS_FAILED(rv)) return rv;

        bool stripWhitespace = false;
        if (node->mType == nsXULPrototypeNode::eType_Element) {
            mozilla::dom::NodeInfo *nodeInfo =
                static_cast<nsXULPrototypeElement*>(node.get())->mNodeInfo;

            if (nodeInfo->NamespaceEquals(kNameSpaceID_XUL))
                stripWhitespace = !nodeInfo->Equals(nsGkAtoms::label) &&
                                  !nodeInfo->Equals(nsGkAtoms::description);
        }

        
        if (stripWhitespace && ! IsDataInBuffer(mText, mTextLength))
            break;

        
        if (mState != eInDocumentElement || mContextStack.Depth() == 0)
            break;

        nsXULPrototypeText* text = new nsXULPrototypeText();
        if (! text)
            return NS_ERROR_OUT_OF_MEMORY;

        text->mValue.Assign(mText, mTextLength);
        if (stripWhitespace)
            text->mValue.Trim(" \t\n\r");

        
        nsPrototypeArray* children = nullptr;
        rv = mContextStack.GetTopChildren(&children);
        if (NS_FAILED(rv)) return rv;

        
        children->AppendElement(text);
    } while (0);

    
    mTextLength = 0;
    return NS_OK;
}



nsresult
XULContentSinkImpl::NormalizeAttributeString(const char16_t *aExpatName,
                                             nsAttrName &aName)
{
    int32_t nameSpaceID;
    nsCOMPtr<nsIAtom> prefix, localName;
    nsContentUtils::SplitExpatName(aExpatName, getter_AddRefs(prefix),
                                   getter_AddRefs(localName), &nameSpaceID);

    if (nameSpaceID == kNameSpaceID_None) {
        aName.SetTo(localName);

        return NS_OK;
    }

    nsRefPtr<mozilla::dom::NodeInfo> ni;
    ni = mNodeInfoManager->GetNodeInfo(localName, prefix,
                                       nameSpaceID,
                                       nsIDOMNode::ATTRIBUTE_NODE);
    aName.SetTo(ni);

    return NS_OK;
}

nsresult
XULContentSinkImpl::CreateElement(mozilla::dom::NodeInfo *aNodeInfo,
                                  nsXULPrototypeElement** aResult)
{
    nsXULPrototypeElement* element = new nsXULPrototypeElement();
    if (! element)
        return NS_ERROR_OUT_OF_MEMORY;

    element->mNodeInfo    = aNodeInfo;
    
    *aResult = element;
    return NS_OK;
}




NS_IMETHODIMP 
XULContentSinkImpl::HandleStartElement(const char16_t *aName, 
                                       const char16_t **aAtts,
                                       uint32_t aAttsCount, 
                                       uint32_t aLineNumber)
{ 
  
  
  NS_PRECONDITION(mState != eInEpilog, "tag in XUL doc epilog");
  NS_PRECONDITION(aAttsCount % 2 == 0, "incorrect aAttsCount");
  
  aAttsCount /= 2;
  
  if (mState == eInEpilog)
      return NS_ERROR_UNEXPECTED;

  if (mState != eInScript) {
      FlushText();
  }

  int32_t nameSpaceID;
  nsCOMPtr<nsIAtom> prefix, localName;
  nsContentUtils::SplitExpatName(aName, getter_AddRefs(prefix),
                                 getter_AddRefs(localName), &nameSpaceID);

  nsRefPtr<mozilla::dom::NodeInfo> nodeInfo;
  nodeInfo = mNodeInfoManager->GetNodeInfo(localName, prefix, nameSpaceID,
                                           nsIDOMNode::ELEMENT_NODE);
  
  nsresult rv = NS_OK;
  switch (mState) {
  case eInProlog:
      
      rv = OpenRoot(aAtts, aAttsCount, nodeInfo);
      break;

  case eInDocumentElement:
      rv = OpenTag(aAtts, aAttsCount, aLineNumber, nodeInfo);
      break;

  case eInEpilog:
  case eInScript:
      PR_LOG(gContentSinkLog, PR_LOG_WARNING,
             ("xul: warning: unexpected tags in epilog at line %d",
             aLineNumber));
      rv = NS_ERROR_UNEXPECTED; 
      break;
  }

  return rv;
}

NS_IMETHODIMP 
XULContentSinkImpl::HandleEndElement(const char16_t *aName)
{
    
    
    
    nsresult rv;

    nsRefPtr<nsXULPrototypeNode> node;
    rv = mContextStack.GetTopNode(node);

    if (NS_FAILED(rv)) {
      return NS_OK;
    }

    switch (node->mType) {
    case nsXULPrototypeNode::eType_Element: {
        
        
        FlushText();

        
        nsPrototypeArray* children = nullptr;
        rv = mContextStack.GetTopChildren(&children);
        if (NS_FAILED(rv)) return rv;

        nsXULPrototypeElement* element =
          static_cast<nsXULPrototypeElement*>(node.get());

        int32_t count = children->Length();
        if (count) {
            element->mChildren.SetCapacity(count);

            for (int32_t i = 0; i < count; ++i)
                element->mChildren.AppendElement(children->ElementAt(i));

        }
    }
    break;

    case nsXULPrototypeNode::eType_Script: {
        nsXULPrototypeScript* script =
            static_cast<nsXULPrototypeScript*>(node.get());

        
        if (!script->mSrcURI && !script->GetScriptObject()) {
            nsCOMPtr<nsIDocument> doc = do_QueryReferent(mDocument);

            script->mOutOfLine = false;
            if (doc)
                script->Compile(mText, mTextLength, mDocumentURL,
                                script->mLineNo, doc);
        }

        FlushText(false);
    }
    break;

    default:
        NS_ERROR("didn't expect that");
        break;
    }

    rv = mContextStack.Pop(&mState);
    NS_ASSERTION(NS_SUCCEEDED(rv), "context stack corrupted");
    if (NS_FAILED(rv)) return rv;

    if (mContextStack.Depth() == 0) {
        
        
        NS_ASSERTION(node->mType == nsXULPrototypeNode::eType_Element, "root is not an element");
        if (node->mType != nsXULPrototypeNode::eType_Element)
            return NS_ERROR_UNEXPECTED;

        
        
        
        nsXULPrototypeElement* element =
            static_cast<nsXULPrototypeElement*>(node.get());

        mPrototype->SetRootElement(element);
        mState = eInEpilog;
    }

    return NS_OK;
}

NS_IMETHODIMP 
XULContentSinkImpl::HandleComment(const char16_t *aName)
{
   FlushText();
   return NS_OK;
}

NS_IMETHODIMP 
XULContentSinkImpl::HandleCDataSection(const char16_t *aData, uint32_t aLength)
{
    FlushText();
    return AddText(aData, aLength);
}

NS_IMETHODIMP 
XULContentSinkImpl::HandleDoctypeDecl(const nsAString & aSubset, 
                                      const nsAString & aName, 
                                      const nsAString & aSystemId, 
                                      const nsAString & aPublicId,
                                      nsISupports* aCatalogData)
{
    return NS_OK;
}

NS_IMETHODIMP 
XULContentSinkImpl::HandleCharacterData(const char16_t *aData, 
                                        uint32_t aLength)
{
  if (aData && mState != eInProlog && mState != eInEpilog) {
    return AddText(aData, aLength);
  }
  return NS_OK;
}

NS_IMETHODIMP 
XULContentSinkImpl::HandleProcessingInstruction(const char16_t *aTarget, 
                                                const char16_t *aData)
{
    FlushText();

    const nsDependentString target(aTarget);
    const nsDependentString data(aData);

    
    nsRefPtr<nsXULPrototypePI> pi = new nsXULPrototypePI();
    if (!pi)
        return NS_ERROR_OUT_OF_MEMORY;

    pi->mTarget = target;
    pi->mData = data;

    if (mState == eInProlog) {
        
        return mPrototype->AddProcessingInstruction(pi);
    }

    nsresult rv;
    nsPrototypeArray* children = nullptr;
    rv = mContextStack.GetTopChildren(&children);
    if (NS_FAILED(rv)) {
        return rv;
    }

    if (!children->AppendElement(pi)) {
        return NS_ERROR_OUT_OF_MEMORY;
    }

    return NS_OK;
}


NS_IMETHODIMP
XULContentSinkImpl::HandleXMLDeclaration(const char16_t *aVersion,
                                         const char16_t *aEncoding,
                                         int32_t aStandalone)
{
  return NS_OK;
}


NS_IMETHODIMP
XULContentSinkImpl::ReportError(const char16_t* aErrorText, 
                                const char16_t* aSourceText,
                                nsIScriptError *aError,
                                bool *_retval)
{
  NS_PRECONDITION(aError && aSourceText && aErrorText, "Check arguments!!!");

  
  *_retval = true;

  nsresult rv = NS_OK;

  
  
  mContextStack.Clear();

  mState = eInProlog;

  
  
  
  mTextLength = 0;

  nsCOMPtr<nsIXULDocument> doc = do_QueryReferent(mDocument);
  if (doc && !doc->OnDocumentParserError()) {
    
    return NS_OK;
  }

  const char16_t* noAtts[] = { 0, 0 };

  NS_NAMED_LITERAL_STRING(errorNs,
                          "http://www.mozilla.org/newlayout/xml/parsererror.xml");

  nsAutoString parsererror(errorNs);
  parsererror.Append((char16_t)0xFFFF);
  parsererror.AppendLiteral("parsererror");
  
  rv = HandleStartElement(parsererror.get(), noAtts, 0, 0);
  NS_ENSURE_SUCCESS(rv,rv);

  rv = HandleCharacterData(aErrorText, NS_strlen(aErrorText));
  NS_ENSURE_SUCCESS(rv,rv);  
  
  nsAutoString sourcetext(errorNs);
  sourcetext.Append((char16_t)0xFFFF);
  sourcetext.AppendLiteral("sourcetext");

  rv = HandleStartElement(sourcetext.get(), noAtts, 0, 0);
  NS_ENSURE_SUCCESS(rv,rv);
  
  rv = HandleCharacterData(aSourceText, NS_strlen(aSourceText));
  NS_ENSURE_SUCCESS(rv,rv);
  
  rv = HandleEndElement(sourcetext.get());
  NS_ENSURE_SUCCESS(rv,rv); 
  
  rv = HandleEndElement(parsererror.get());
  NS_ENSURE_SUCCESS(rv,rv);

  return rv;
}

nsresult
XULContentSinkImpl::OpenRoot(const char16_t** aAttributes, 
                             const uint32_t aAttrLen, 
                             mozilla::dom::NodeInfo *aNodeInfo)
{
    NS_ASSERTION(mState == eInProlog, "how'd we get here?");
    if (mState != eInProlog)
        return NS_ERROR_UNEXPECTED;

    nsresult rv;

    if (aNodeInfo->Equals(nsGkAtoms::script, kNameSpaceID_XHTML) || 
        aNodeInfo->Equals(nsGkAtoms::script, kNameSpaceID_XUL)) {
        PR_LOG(gContentSinkLog, PR_LOG_ERROR,
               ("xul: script tag not allowed as root content element"));

        return NS_ERROR_UNEXPECTED;
    }

    
    nsXULPrototypeElement* element;
    rv = CreateElement(aNodeInfo, &element);

    if (NS_FAILED(rv)) {
#ifdef PR_LOGGING
        if (PR_LOG_TEST(gContentSinkLog, PR_LOG_ERROR)) {
            nsAutoString anodeC;
            aNodeInfo->GetName(anodeC);
            PR_LOG(gContentSinkLog, PR_LOG_ERROR,
                   ("xul: unable to create element '%s' at line %d",
                    NS_ConvertUTF16toUTF8(anodeC).get(),
                    -1)); 
        }
#endif

        return rv;
    }

    
    
    rv = mContextStack.Push(element, mState);
    if (NS_FAILED(rv)) {
        element->Release();
        return rv;
    }

    
    rv = AddAttributes(aAttributes, aAttrLen, element);
    if (NS_FAILED(rv)) return rv;

    mState = eInDocumentElement;
    return NS_OK;
}

nsresult
XULContentSinkImpl::OpenTag(const char16_t** aAttributes, 
                            const uint32_t aAttrLen,
                            const uint32_t aLineNumber,
                            mozilla::dom::NodeInfo *aNodeInfo)
{
    nsresult rv;

    
    nsXULPrototypeElement* element;
    rv = CreateElement(aNodeInfo, &element);

    if (NS_FAILED(rv)) {
#ifdef PR_LOGGING
        if (PR_LOG_TEST(gContentSinkLog, PR_LOG_ERROR)) {
            nsAutoString anodeC;
            aNodeInfo->GetName(anodeC);
            PR_LOG(gContentSinkLog, PR_LOG_ERROR,
                   ("xul: unable to create element '%s' at line %d",
                    NS_ConvertUTF16toUTF8(anodeC).get(),
                    aLineNumber));
        }
#endif

        return rv;
    }

    
    nsPrototypeArray* children = nullptr;
    rv = mContextStack.GetTopChildren(&children);
    if (NS_FAILED(rv)) {
        delete element;
        return rv;
    }

    
    rv = AddAttributes(aAttributes, aAttrLen, element);
    if (NS_FAILED(rv)) return rv;

    children->AppendElement(element);

    if (aNodeInfo->Equals(nsGkAtoms::script, kNameSpaceID_XHTML) || 
        aNodeInfo->Equals(nsGkAtoms::script, kNameSpaceID_XUL)) {
        
        rv = OpenScript(aAttributes, aLineNumber);
        NS_ENSURE_SUCCESS(rv, rv);

        NS_ASSERTION(mState == eInScript || mState == eInDocumentElement,
                     "Unexpected state");
        if (mState == eInScript) {
            
            
            return NS_OK;
        }
    }

    
    
    rv = mContextStack.Push(element, mState);
    if (NS_FAILED(rv)) return rv;

    mState = eInDocumentElement;
    return NS_OK;
}

nsresult
XULContentSinkImpl::OpenScript(const char16_t** aAttributes,
                               const uint32_t aLineNumber)
{
  uint32_t langID = nsIProgrammingLanguage::JAVASCRIPT;
  uint32_t version = JSVERSION_LATEST;
  nsresult rv;

  
  nsAutoString src;
  while (*aAttributes) {
      const nsDependentString key(aAttributes[0]);
      if (key.EqualsLiteral("src")) {
          src.Assign(aAttributes[1]);
      } else if (key.EqualsLiteral("type")) {
          nsDependentString str(aAttributes[1]);
          nsContentTypeParser parser(str);
          nsAutoString mimeType;
          rv = parser.GetType(mimeType);
          if (NS_FAILED(rv)) {
              if (rv == NS_ERROR_INVALID_ARG) {
                  
                  
                  return NS_OK;
              }
              
              NS_ENSURE_SUCCESS(rv, rv);
          }

          if (nsContentUtils::IsJavascriptMIMEType(mimeType)) {
              langID = nsIProgrammingLanguage::JAVASCRIPT;
              version = JSVERSION_LATEST;

              
              nsAutoString versionName;
              rv = parser.GetParameter("version", versionName);

              if (NS_SUCCEEDED(rv)) {
                  version = nsContentUtils::ParseJavascriptVersion(versionName);
              } else if (rv != NS_ERROR_INVALID_ARG) {
                  return rv;
              }
          } else {
              langID = nsIProgrammingLanguage::UNKNOWN;
          }
      } else if (key.EqualsLiteral("language")) {
          
          
          
          nsAutoString lang(aAttributes[1]);
          if (nsContentUtils::IsJavaScriptLanguage(lang)) {
              version = JSVERSION_DEFAULT;
              langID = nsIProgrammingLanguage::JAVASCRIPT;
          }
      }
      aAttributes += 2;
  }

  nsCOMPtr<nsIDocument> doc(do_QueryReferent(mDocument));

  
  if (langID == nsIProgrammingLanguage::UNKNOWN) {
      return NS_OK;
  }

  nsCOMPtr<nsIScriptGlobalObject> globalObject;
  if (doc)
      globalObject = do_QueryInterface(doc->GetWindow());
  nsRefPtr<nsXULPrototypeScript> script =
      new nsXULPrototypeScript(aLineNumber, version);
  if (! script)
      return NS_ERROR_OUT_OF_MEMORY;

  
  if (! src.IsEmpty()) {
      
      rv = NS_NewURI(getter_AddRefs(script->mSrcURI), src, nullptr, mDocumentURL);

      
      
      
      if (NS_SUCCEEDED(rv)) {
          if (!mSecMan)
              mSecMan = do_GetService(NS_SCRIPTSECURITYMANAGER_CONTRACTID, &rv);
          if (NS_SUCCEEDED(rv)) {
              nsCOMPtr<nsIDocument> doc = do_QueryReferent(mDocument, &rv);

              if (NS_SUCCEEDED(rv)) {
                  rv = mSecMan->
                      CheckLoadURIWithPrincipal(doc->NodePrincipal(),
                                                script->mSrcURI,
                                                nsIScriptSecurityManager::ALLOW_CHROME);
              }
          }
      }

      if (NS_FAILED(rv)) {
          return rv;
      }

      
      
      
      
      script->DeserializeOutOfLine(nullptr, mPrototype);
  }

  nsPrototypeArray* children = nullptr;
  rv = mContextStack.GetTopChildren(&children);
  if (NS_FAILED(rv)) {
      return rv;
  }

  children->AppendElement(script);

  mConstrainSize = false;

  mContextStack.Push(script, mState);
  mState = eInScript;

  return NS_OK;
}

nsresult
XULContentSinkImpl::AddAttributes(const char16_t** aAttributes, 
                                  const uint32_t aAttrLen, 
                                  nsXULPrototypeElement* aElement)
{
  
  nsresult rv;

  
  nsXULPrototypeAttribute* attrs = nullptr;
  if (aAttrLen > 0) {
    attrs = new nsXULPrototypeAttribute[aAttrLen];
    if (! attrs)
      return NS_ERROR_OUT_OF_MEMORY;
  }

  aElement->mAttributes    = attrs;
  aElement->mNumAttributes = aAttrLen;

  
  uint32_t i;
  for (i = 0; i < aAttrLen; ++i) {
      rv = NormalizeAttributeString(aAttributes[i * 2], attrs[i].mName);
      NS_ENSURE_SUCCESS(rv, rv);

      rv = aElement->SetAttrAt(i, nsDependentString(aAttributes[i * 2 + 1]),
                               mDocumentURL);
      NS_ENSURE_SUCCESS(rv, rv);

#ifdef PR_LOGGING
      if (PR_LOG_TEST(gContentSinkLog, PR_LOG_DEBUG)) {
          nsAutoString extraWhiteSpace;
          int32_t cnt = mContextStack.Depth();
          while (--cnt >= 0)
              extraWhiteSpace.AppendLiteral("  ");
          nsAutoString qnameC,valueC;
          qnameC.Assign(aAttributes[0]);
          valueC.Assign(aAttributes[1]);
          PR_LOG(gContentSinkLog, PR_LOG_DEBUG,
                 ("xul: %.5d. %s    %s=%s",
                  -1, 
                  NS_ConvertUTF16toUTF8(extraWhiteSpace).get(),
                  NS_ConvertUTF16toUTF8(qnameC).get(),
                  NS_ConvertUTF16toUTF8(valueC).get()));
      }
#endif
  }

  return NS_OK;
}

nsresult
XULContentSinkImpl::AddText(const char16_t* aText, 
                            int32_t aLength)
{
  
  if (0 == mTextSize) {
      mText = (char16_t *) malloc(sizeof(char16_t) * 4096);
      if (nullptr == mText) {
          return NS_ERROR_OUT_OF_MEMORY;
      }
      mTextSize = 4096;
  }

  
  int32_t offset = 0;
  while (0 != aLength) {
    int32_t amount = mTextSize - mTextLength;
    if (amount > aLength) {
        amount = aLength;
    }
    if (0 == amount) {
      if (mConstrainSize) {
        nsresult rv = FlushText();
        if (NS_OK != rv) {
            return rv;
        }
      } else {
        mTextSize += aLength;
        mText = (char16_t *) realloc(mText, sizeof(char16_t) * mTextSize);
        if (nullptr == mText) {
            return NS_ERROR_OUT_OF_MEMORY;
        }
      }
    }
    memcpy(&mText[mTextLength],aText + offset, sizeof(char16_t) * amount);
    
    mTextLength += amount;
    offset += amount;
    aLength -= amount;
  }

  return NS_OK;
}
