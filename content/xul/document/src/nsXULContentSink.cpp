
















































#include "nsXULContentSink.h"
#include "nsCOMPtr.h"
#include "nsForwardReference.h"
#include "nsIContentSink.h"
#include "nsIDOMDocument.h"
#include "nsIDOMEventListener.h"
#include "nsIDOMHTMLFormElement.h"
#include "nsIDOMXULDocument.h"
#include "nsIDocument.h"
#include "nsIFormControl.h"
#include "nsHTMLStyleSheet.h"
#include "nsINameSpaceManager.h"
#include "nsINodeInfo.h"
#include "nsIParser.h"
#include "nsIPresShell.h"
#include "nsIScriptContext.h"
#include "nsIScriptRuntime.h"
#include "nsIScriptGlobalObject.h"
#include "nsIServiceManager.h"
#include "nsIURL.h"
#include "nsIViewManager.h"
#include "nsIXULDocument.h"
#include "nsIScriptSecurityManager.h"
#include "nsLayoutCID.h"
#include "nsNetUtil.h"
#include "nsRDFCID.h"
#include "nsParserUtils.h"
#include "nsXPIDLString.h"
#include "nsReadableUtils.h"
#include "nsXULElement.h"
#include "prlog.h"
#include "prmem.h"
#include "jscntxt.h"  
#include "nsCRT.h"

#include "nsXULPrototypeDocument.h"     
#include "nsICSSLoader.h"

#include "nsUnicharUtils.h"
#include "nsGkAtoms.h"
#include "nsContentUtils.h"
#include "nsAttrName.h"
#include "nsXMLContentSink.h"
#include "nsIConsoleService.h"
#include "nsIScriptError.h"

#ifdef PR_LOGGING
static PRLogModuleInfo* gLog;
#endif



XULContentSinkImpl::ContextStack::ContextStack()
    : mTop(nsnull), mDepth(0)
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

nsresult
XULContentSinkImpl::ContextStack::GetTopNodeScriptType(PRUint32 *aScriptType)
{
    if (mDepth == 0)
        return NS_ERROR_UNEXPECTED;

    
    
    nsresult rv = NS_OK;
    nsRefPtr<nsXULPrototypeNode> node;
    rv = GetTopNode(node);
    if (NS_FAILED(rv)) return rv;
    switch (node->mType) {
        case nsXULPrototypeNode::eType_Element: {
            nsXULPrototypeElement *parent =
                reinterpret_cast<nsXULPrototypeElement*>(node.get());
            *aScriptType = parent->mScriptTypeID;
            break;
        }
        case nsXULPrototypeNode::eType_Script: {
            nsXULPrototypeScript *parent =
                reinterpret_cast<nsXULPrototypeScript*>(node.get());
            *aScriptType = parent->mScriptObject.mLangID;
            break;
        }
        default: {
            NS_WARNING("Unexpected parent node type");
            rv = NS_ERROR_UNEXPECTED;
        }
    }
    return rv;
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

  mTop = nsnull;
  mDepth = 0;
}




XULContentSinkImpl::XULContentSinkImpl()
    : mText(nsnull),
      mTextLength(0),
      mTextSize(0),
      mConstrainSize(PR_TRUE),
      mState(eInProlog),
      mParser(nsnull)
{

#ifdef PR_LOGGING
    if (! gLog)
        gLog = PR_NewLogModule("nsXULContentSink");
#endif
}


XULContentSinkImpl::~XULContentSinkImpl()
{
    NS_IF_RELEASE(mParser); 

    
    NS_ASSERTION(mContextStack.Depth() == 0, "Context stack not empty?");
    mContextStack.Clear();

    PR_FREEIF(mText);
}




NS_IMPL_ISUPPORTS3(XULContentSinkImpl,
                   nsIXMLContentSink,
                   nsIContentSink,
                   nsIExpatSink)




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
XULContentSinkImpl::DidBuildModel(void)
{
    nsCOMPtr<nsIDocument> doc = do_QueryReferent(mDocument);
    if (doc) {
        doc->EndLoad();
        mDocument = nsnull;
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
XULContentSinkImpl::SetParser(nsIParser* aParser)
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
    NS_PRECONDITION(aDocument != nsnull, "null ptr");
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







PRBool
XULContentSinkImpl::IsDataInBuffer(PRUnichar* buffer, PRInt32 length)
{
    for (PRInt32 i = 0; i < length; ++i) {
        if (buffer[i] == ' ' ||
            buffer[i] == '\t' ||
            buffer[i] == '\n' ||
            buffer[i] == '\r')
            continue;

        return PR_TRUE;
    }
    return PR_FALSE;
}


nsresult
XULContentSinkImpl::FlushText(PRBool aCreateTextNode)
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

        PRBool stripWhitespace = PR_FALSE;
        if (node->mType == nsXULPrototypeNode::eType_Element) {
            nsINodeInfo *nodeInfo =
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

        
        nsPrototypeArray* children = nsnull;
        rv = mContextStack.GetTopChildren(&children);
        if (NS_FAILED(rv)) return rv;

        
        children->AppendElement(text);
    } while (0);

    
    mTextLength = 0;
    return NS_OK;
}



nsresult
XULContentSinkImpl::NormalizeAttributeString(const PRUnichar *aExpatName,
                                             nsAttrName &aName)
{
    PRInt32 nameSpaceID;
    nsCOMPtr<nsIAtom> prefix, localName;
    nsContentUtils::SplitExpatName(aExpatName, getter_AddRefs(prefix),
                                   getter_AddRefs(localName), &nameSpaceID);

    if (nameSpaceID == kNameSpaceID_None) {
        aName.SetTo(localName);

        return NS_OK;
    }

    nsCOMPtr<nsINodeInfo> ni;
    ni = mNodeInfoManager->GetNodeInfo(localName, prefix,
                                       nameSpaceID);
    NS_ENSURE_TRUE(ni, NS_ERROR_OUT_OF_MEMORY);

    aName.SetTo(ni);

    return NS_OK;
}

nsresult
XULContentSinkImpl::CreateElement(nsINodeInfo *aNodeInfo,
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
XULContentSinkImpl::HandleStartElement(const PRUnichar *aName, 
                                       const PRUnichar **aAtts,
                                       PRUint32 aAttsCount, 
                                       PRInt32 aIndex, 
                                       PRUint32 aLineNumber)
{ 
  
  
  NS_PRECONDITION(mState != eInEpilog, "tag in XUL doc epilog");
  NS_PRECONDITION(aIndex >= -1, "Bogus aIndex");
  NS_PRECONDITION(aAttsCount % 2 == 0, "incorrect aAttsCount");
  
  aAttsCount /= 2;
  
  if (mState == eInEpilog)
      return NS_ERROR_UNEXPECTED;

  if (mState != eInScript) {
      FlushText();
  }

  PRInt32 nameSpaceID;
  nsCOMPtr<nsIAtom> prefix, localName;
  nsContentUtils::SplitExpatName(aName, getter_AddRefs(prefix),
                                 getter_AddRefs(localName), &nameSpaceID);

  nsCOMPtr<nsINodeInfo> nodeInfo;
  nodeInfo = mNodeInfoManager->GetNodeInfo(localName, prefix, nameSpaceID);
  NS_ENSURE_TRUE(nodeInfo, NS_ERROR_OUT_OF_MEMORY);
  
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
      PR_LOG(gLog, PR_LOG_WARNING,
             ("xul: warning: unexpected tags in epilog at line %d",
             aLineNumber));
      rv = NS_ERROR_UNEXPECTED; 
      break;
  }

  
  if (aIndex != -1 && NS_SUCCEEDED(rv)) {
    nsCOMPtr<nsIAtom> IDAttr = do_GetAtom(aAtts[aIndex]);

    if (IDAttr) {
      nodeInfo->SetIDAttributeAtom(IDAttr);
    }
  }

  return rv;
}

NS_IMETHODIMP 
XULContentSinkImpl::HandleEndElement(const PRUnichar *aName)
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

        
        nsPrototypeArray* children = nsnull;
        rv = mContextStack.GetTopChildren(&children);
        if (NS_FAILED(rv)) return rv;

        nsXULPrototypeElement* element =
          static_cast<nsXULPrototypeElement*>(node.get());

        PRInt32 count = children->Length();
        if (count) {
            if (!element->mChildren.SetCapacity(count))
                return NS_ERROR_OUT_OF_MEMORY;

            for (PRInt32 i = 0; i < count; ++i)
                element->mChildren.AppendElement(children->ElementAt(i));

        }
    }
    break;

    case nsXULPrototypeNode::eType_Script: {
        nsXULPrototypeScript* script =
            static_cast<nsXULPrototypeScript*>(node.get());

        
        if (! script->mSrcURI && ! script->mScriptObject.mObject) {
            nsCOMPtr<nsIDocument> doc = do_QueryReferent(mDocument);

            script->mOutOfLine = PR_FALSE;
            if (doc)
                script->Compile(mText, mTextLength, mDocumentURL,
                                script->mLineNo, doc, mPrototype);
        }

        FlushText(PR_FALSE);
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
XULContentSinkImpl::HandleComment(const PRUnichar *aName)
{
   FlushText();
   return NS_OK;
}

NS_IMETHODIMP 
XULContentSinkImpl::HandleCDataSection(const PRUnichar *aData, PRUint32 aLength)
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
XULContentSinkImpl::HandleCharacterData(const PRUnichar *aData, 
                                        PRUint32 aLength)
{
  if (aData && mState != eInProlog && mState != eInEpilog) {
    return AddText(aData, aLength);
  }
  return NS_OK;
}

NS_IMETHODIMP 
XULContentSinkImpl::HandleProcessingInstruction(const PRUnichar *aTarget, 
                                                const PRUnichar *aData)
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
    nsPrototypeArray* children = nsnull;
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
XULContentSinkImpl::HandleXMLDeclaration(const PRUnichar *aVersion,
                                         const PRUnichar *aEncoding,
                                         PRInt32 aStandalone)
{
  return NS_OK;
}


NS_IMETHODIMP
XULContentSinkImpl::ReportError(const PRUnichar* aErrorText, 
                                const PRUnichar* aSourceText,
                                nsIScriptError *aError,
                                PRBool *_retval)
{
  NS_PRECONDITION(aError && aSourceText && aErrorText, "Check arguments!!!");

  
  *_retval = PR_TRUE;

  nsresult rv = NS_OK;

  
  
  mContextStack.Clear();

  mState = eInProlog;

  
  
  
  mTextLength = 0;

  nsCOMPtr<nsIXULDocument> doc = do_QueryReferent(mDocument);
  if (doc && !doc->OnDocumentParserError()) {
    
    return NS_OK;
  }

  const PRUnichar* noAtts[] = { 0, 0 };

  NS_NAMED_LITERAL_STRING(errorNs,
                          "http://www.mozilla.org/newlayout/xml/parsererror.xml");

  nsAutoString parsererror(errorNs);
  parsererror.Append((PRUnichar)0xFFFF);
  parsererror.AppendLiteral("parsererror");
  
  rv = HandleStartElement(parsererror.get(), noAtts, 0, -1, 0);
  NS_ENSURE_SUCCESS(rv,rv);

  rv = HandleCharacterData(aErrorText, nsCRT::strlen(aErrorText));
  NS_ENSURE_SUCCESS(rv,rv);  
  
  nsAutoString sourcetext(errorNs);
  sourcetext.Append((PRUnichar)0xFFFF);
  sourcetext.AppendLiteral("sourcetext");

  rv = HandleStartElement(sourcetext.get(), noAtts, 0, -1, 0);
  NS_ENSURE_SUCCESS(rv,rv);
  
  rv = HandleCharacterData(aSourceText, nsCRT::strlen(aSourceText));
  NS_ENSURE_SUCCESS(rv,rv);
  
  rv = HandleEndElement(sourcetext.get());
  NS_ENSURE_SUCCESS(rv,rv); 
  
  rv = HandleEndElement(parsererror.get());
  NS_ENSURE_SUCCESS(rv,rv);

  return rv;
}

nsresult
XULContentSinkImpl::SetElementScriptType(nsXULPrototypeElement* element,
                                         const PRUnichar** aAttributes, 
                                         const PRUint32 aAttrLen)
{
    
    nsresult rv = NS_OK;
    PRUint32 i;
    PRBool found = PR_FALSE;
    for (i=0;i<aAttrLen;i++) {
        const nsDependentString key(aAttributes[i*2]);
        if (key.EqualsLiteral("script-type")) {
            const nsDependentString value(aAttributes[i*2+1]);
            if (!value.IsEmpty()) {
                nsCOMPtr<nsIScriptRuntime> runtime;
                rv = NS_GetScriptRuntime(value, getter_AddRefs(runtime));
                if (NS_SUCCEEDED(rv))
                    element->mScriptTypeID = runtime->GetScriptTypeID();
                else {
                    
                    NS_WARNING("Failed to load the node's script language!");
                    
                    
                    NS_ASSERTION(element->mScriptTypeID == nsIProgrammingLanguage::UNKNOWN,
                                 "Default script type should be unknown");
                }
                found = PR_TRUE;
                break;
            }
        }
    }
    
    
    if (!found) {
        if (mContextStack.Depth() == 0) {
            
            element->mScriptTypeID = nsIProgrammingLanguage::JAVASCRIPT;
        } else {
            
            
            
            PRUint32 scriptId = 0;
            rv = mContextStack.GetTopNodeScriptType(&scriptId);
            element->mScriptTypeID = scriptId;
        }
    }
    return rv;
}

nsresult
XULContentSinkImpl::OpenRoot(const PRUnichar** aAttributes, 
                             const PRUint32 aAttrLen, 
                             nsINodeInfo *aNodeInfo)
{
    NS_ASSERTION(mState == eInProlog, "how'd we get here?");
    if (mState != eInProlog)
        return NS_ERROR_UNEXPECTED;

    nsresult rv;

    if (aNodeInfo->Equals(nsGkAtoms::script, kNameSpaceID_XHTML) || 
        aNodeInfo->Equals(nsGkAtoms::script, kNameSpaceID_XUL)) {
        PR_LOG(gLog, PR_LOG_ERROR,
               ("xul: script tag not allowed as root content element"));

        return NS_ERROR_UNEXPECTED;
    }

    
    nsXULPrototypeElement* element;
    rv = CreateElement(aNodeInfo, &element);

    if (NS_FAILED(rv)) {
#ifdef PR_LOGGING
        if (PR_LOG_TEST(gLog, PR_LOG_ERROR)) {
            nsAutoString anodeC;
            aNodeInfo->GetName(anodeC);
            PR_LOG(gLog, PR_LOG_ERROR,
                   ("xul: unable to create element '%s' at line %d",
                    NS_ConvertUTF16toUTF8(anodeC).get(),
                    -1)); 
        }
#endif

        return rv;
    }

    
    rv = SetElementScriptType(element, aAttributes, aAttrLen);
    if (NS_FAILED(rv)) return rv;

    
    
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
XULContentSinkImpl::OpenTag(const PRUnichar** aAttributes, 
                            const PRUint32 aAttrLen,
                            const PRUint32 aLineNumber,
                            nsINodeInfo *aNodeInfo)
{
    nsresult rv;

    
    nsXULPrototypeElement* element;
    rv = CreateElement(aNodeInfo, &element);

    if (NS_FAILED(rv)) {
#ifdef PR_LOGGING
        if (PR_LOG_TEST(gLog, PR_LOG_ERROR)) {
            nsAutoString anodeC;
            aNodeInfo->GetName(anodeC);
            PR_LOG(gLog, PR_LOG_ERROR,
                   ("xul: unable to create element '%s' at line %d",
                    NS_ConvertUTF16toUTF8(anodeC).get(),
                    aLineNumber));
        }
#endif

        return rv;
    }

    
    nsPrototypeArray* children = nsnull;
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
        
        
        
        element->mScriptTypeID = nsIProgrammingLanguage::JAVASCRIPT;
        rv = OpenScript(aAttributes, aLineNumber);
        NS_ENSURE_SUCCESS(rv, rv);

        NS_ASSERTION(mState == eInScript || mState == eInDocumentElement,
                     "Unexpected state");
        if (mState == eInScript) {
            
            
            return NS_OK;
        }
    }

    
    rv = SetElementScriptType(element, aAttributes, aAttrLen);
    if (NS_FAILED(rv)) return rv;

    
    
    rv = mContextStack.Push(element, mState);
    if (NS_FAILED(rv)) return rv;

    mState = eInDocumentElement;
    return NS_OK;
}

nsresult
XULContentSinkImpl::OpenScript(const PRUnichar** aAttributes,
                               const PRUint32 aLineNumber)
{
  PRUint32 langID;
  nsresult rv = mContextStack.GetTopNodeScriptType(&langID);
  if (NS_FAILED(rv)) return rv;
  PRUint32 version = 0;

  
  nsAutoString src;
  while (*aAttributes) {
      const nsDependentString key(aAttributes[0]);
      if (key.EqualsLiteral("src")) {
          src.Assign(aAttributes[1]);
      }
      else if (key.EqualsLiteral("type")) {
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

          
          
          
          
          
          static const char *jsTypes[] = {
              "application/x-javascript",
              "text/javascript",
              "text/ecmascript",
              "application/javascript",
              "application/ecmascript",
              nsnull
          };

          PRBool isJavaScript = PR_FALSE;
          for (PRInt32 i = 0; jsTypes[i]; i++) {
              if (mimeType.LowerCaseEqualsASCII(jsTypes[i])) {
                  isJavaScript = PR_TRUE;
                  break;
              }
          }

          if (isJavaScript) {
              langID = nsIProgrammingLanguage::JAVASCRIPT;
              version = JSVERSION_LATEST;
          } else {
              
              nsCOMPtr<nsIScriptRuntime> runtime;
              rv = NS_GetScriptRuntime(mimeType, getter_AddRefs(runtime));
              if (NS_FAILED(rv) || runtime == nsnull) {
                  
                  NS_WARNING("Failed to find a scripting language");
                  langID = nsIProgrammingLanguage::UNKNOWN;
              } else
                  langID = runtime->GetScriptTypeID();
          }

          if (langID != nsIProgrammingLanguage::UNKNOWN) {
            
            nsAutoString versionName;
            rv = parser.GetParameter("version", versionName);
            if (NS_FAILED(rv)) {
              if (rv != NS_ERROR_INVALID_ARG)
                return rv;
              
            } else {
              nsCOMPtr<nsIScriptRuntime> runtime;
              rv = NS_GetScriptRuntimeByID(langID, getter_AddRefs(runtime));
              if (NS_FAILED(rv))
                return rv;
              rv = runtime->ParseVersion(versionName, &version);
              if (NS_FAILED(rv)) {
                NS_WARNING("This script language version is not supported - ignored");
                langID = nsIProgrammingLanguage::UNKNOWN;
              }
            }
          }
          
          if (langID == nsIProgrammingLanguage::JAVASCRIPT) {
              
              
              
              
              version |= JSVERSION_HAS_XML;

              nsAutoString value;
              rv = parser.GetParameter("e4x", value);
              if (NS_FAILED(rv)) {
                  if (rv != NS_ERROR_INVALID_ARG)
                      return rv;
              } else {
                  if (value.Length() == 1 && value[0] == '0')
                    version &= ~JSVERSION_HAS_XML;
              }
          }
      }
      else if (key.EqualsLiteral("language")) {
          
          
          
          nsAutoString lang(aAttributes[1]);
          if (nsParserUtils::IsJavaScriptLanguage(lang, &version)) {
              langID = nsIProgrammingLanguage::JAVASCRIPT;

              
              
              version |= JSVERSION_HAS_XML;
          }
      }
      aAttributes += 2;
  }
  
  
  
  
  
  
  
  
  nsCOMPtr<nsIDocument> doc(do_QueryReferent(mDocument));
  if (langID != nsIProgrammingLanguage::UNKNOWN && 
      langID != nsIProgrammingLanguage::JAVASCRIPT &&
      doc && !nsContentUtils::IsChromeDoc(doc)) {
      langID = nsIProgrammingLanguage::UNKNOWN;
      NS_WARNING("Non JS language called from non chrome - ignored");
  }
  
  if (langID != nsIProgrammingLanguage::UNKNOWN) {
      nsIScriptGlobalObject* globalObject = nsnull; 
      if (doc)
          globalObject = doc->GetScriptGlobalObject();
      nsRefPtr<nsXULPrototypeScript> script =
          new nsXULPrototypeScript(langID, aLineNumber, version);
      if (! script)
          return NS_ERROR_OUT_OF_MEMORY;

      
      if (! src.IsEmpty()) {
          
          rv = NS_NewURI(getter_AddRefs(script->mSrcURI), src, nsnull, mDocumentURL);

          
          
          
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

          
          
          
          
          if (globalObject)
                script->DeserializeOutOfLine(nsnull, globalObject);
      }

      nsPrototypeArray* children = nsnull;
      rv = mContextStack.GetTopChildren(&children);
      if (NS_FAILED(rv)) {
          return rv;
      }

      children->AppendElement(script);

      mConstrainSize = PR_FALSE;

      mContextStack.Push(script, mState);
      mState = eInScript;
  }

  return NS_OK;
}

nsresult
XULContentSinkImpl::AddAttributes(const PRUnichar** aAttributes, 
                                  const PRUint32 aAttrLen, 
                                  nsXULPrototypeElement* aElement)
{
  
  nsresult rv;

  
  nsXULPrototypeAttribute* attrs = nsnull;
  if (aAttrLen > 0) {
    attrs = new nsXULPrototypeAttribute[aAttrLen];
    if (! attrs)
      return NS_ERROR_OUT_OF_MEMORY;
  }

  aElement->mAttributes    = attrs;
  aElement->mNumAttributes = aAttrLen;

  
  PRUint32 i;
  for (i = 0; i < aAttrLen; ++i) {
      rv = NormalizeAttributeString(aAttributes[i * 2], attrs[i].mName);
      NS_ENSURE_SUCCESS(rv, rv);

      rv = aElement->SetAttrAt(i, nsDependentString(aAttributes[i * 2 + 1]),
                               mDocumentURL);
      NS_ENSURE_SUCCESS(rv, rv);

#ifdef PR_LOGGING
      if (PR_LOG_TEST(gLog, PR_LOG_DEBUG)) {
          nsAutoString extraWhiteSpace;
          PRInt32 cnt = mContextStack.Depth();
          while (--cnt >= 0)
              extraWhiteSpace.AppendLiteral("  ");
          nsAutoString qnameC,valueC;
          qnameC.Assign(aAttributes[0]);
          valueC.Assign(aAttributes[1]);
          PR_LOG(gLog, PR_LOG_DEBUG,
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
XULContentSinkImpl::AddText(const PRUnichar* aText, 
                            PRInt32 aLength)
{
  
  if (0 == mTextSize) {
      mText = (PRUnichar *) PR_MALLOC(sizeof(PRUnichar) * 4096);
      if (nsnull == mText) {
          return NS_ERROR_OUT_OF_MEMORY;
      }
      mTextSize = 4096;
  }

  
  PRInt32 offset = 0;
  while (0 != aLength) {
    PRInt32 amount = mTextSize - mTextLength;
    if (amount > aLength) {
        amount = aLength;
    }
    if (0 == amount) {
      if (mConstrainSize) {
        nsresult rv = FlushText();
        if (NS_OK != rv) {
            return rv;
        }
      }
      else {
        mTextSize += aLength;
        mText = (PRUnichar *) PR_REALLOC(mText, sizeof(PRUnichar) * mTextSize);
        if (nsnull == mText) {
            return NS_ERROR_OUT_OF_MEMORY;
        }
      }
    }
    memcpy(&mText[mTextLength],aText + offset, sizeof(PRUnichar) * amount);
    
    mTextLength += amount;
    offset += amount;
    aLength -= amount;
  }

  return NS_OK;
}
