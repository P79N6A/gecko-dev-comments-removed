





































#include "nsXBLContentSink.h"
#include "nsIDocument.h"
#include "nsBindingManager.h"
#include "nsIDOMNode.h"
#include "nsIParser.h"
#include "nsGkAtoms.h"
#include "nsINameSpaceManager.h"
#include "nsHTMLTokens.h"
#include "nsIURI.h"
#include "nsTextFragment.h"
#ifdef MOZ_XUL
#include "nsXULElement.h"
#endif
#include "nsXBLProtoImplProperty.h"
#include "nsXBLProtoImplMethod.h"
#include "nsXBLProtoImplField.h"
#include "nsXBLPrototypeBinding.h"
#include "nsContentUtils.h"
#include "nsIConsoleService.h"
#include "nsIScriptError.h"
#include "nsNodeInfoManager.h"
#include "nsINodeInfo.h"
#include "nsIPrincipal.h"

nsresult
NS_NewXBLContentSink(nsIXMLContentSink** aResult,
                     nsIDocument* aDoc,
                     nsIURI* aURI,
                     nsISupports* aContainer)
{
  NS_ENSURE_ARG_POINTER(aResult);

  nsXBLContentSink* it;
  NS_NEWXPCOM(it, nsXBLContentSink);
  NS_ENSURE_TRUE(it, NS_ERROR_OUT_OF_MEMORY);

  nsCOMPtr<nsIXMLContentSink> kungFuDeathGrip = it;
  nsresult rv = it->Init(aDoc, aURI, aContainer);
  NS_ENSURE_SUCCESS(rv, rv);

  return CallQueryInterface(it, aResult);
}

nsXBLContentSink::nsXBLContentSink()
  : mState(eXBL_InDocument),
    mSecondaryState(eXBL_None),
    mDocInfo(nsnull),
    mIsChromeOrResource(PR_FALSE),
    mFoundFirstBinding(PR_FALSE),    
    mBinding(nsnull),
    mHandler(nsnull),
    mImplementation(nsnull),
    mImplMember(nsnull),
    mImplField(nsnull),
    mProperty(nsnull),
    mMethod(nsnull),
    mField(nsnull)
{
  mPrettyPrintXML = PR_FALSE;
}

nsXBLContentSink::~nsXBLContentSink()
{
}

nsresult
nsXBLContentSink::Init(nsIDocument* aDoc,
                       nsIURI* aURI,
                       nsISupports* aContainer)
{
  nsresult rv;
  rv = nsXMLContentSink::Init(aDoc, aURI, aContainer, nsnull);
  return rv;
}

void
nsXBLContentSink::MaybeStartLayout(PRBool aIgnorePendingSheets)
{
  return;
}

nsresult
nsXBLContentSink::FlushText(PRBool aReleaseTextNode)
{
  if (mTextLength != 0) {
    const nsASingleFragmentString& text = Substring(mText, mText+mTextLength);
    if (mState == eXBL_InHandlers) {
      NS_ASSERTION(mBinding, "Must have binding here");
      
      if (mSecondaryState == eXBL_InHandler)
        mHandler->AppendHandlerText(text);
      mTextLength = 0;
      return NS_OK;
    }
    else if (mState == eXBL_InImplementation) {
      NS_ASSERTION(mBinding, "Must have binding here");
      if (mSecondaryState == eXBL_InConstructor ||
          mSecondaryState == eXBL_InDestructor) {
        
        nsXBLProtoImplMethod* method;
        if (mSecondaryState == eXBL_InConstructor)
          method = mBinding->GetConstructor();
        else
          method = mBinding->GetDestructor();

        
        method->AppendBodyText(text);
      }
      else if (mSecondaryState == eXBL_InGetter ||
               mSecondaryState == eXBL_InSetter) {
        
        if (mSecondaryState == eXBL_InGetter)
          mProperty->AppendGetterText(text);
        else
          mProperty->AppendSetterText(text);
      }
      else if (mSecondaryState == eXBL_InBody) {
        
        if (mMethod)
          mMethod->AppendBodyText(text);
      }
      else if (mSecondaryState == eXBL_InField) {
        
        if (mField)
          mField->AppendFieldText(text);
      }
      mTextLength = 0;
      return NS_OK;
    }

    nsIContent* content = GetCurrentContent();
    if (content &&
        (content->NodeInfo()->NamespaceEquals(kNameSpaceID_XBL) ||
         (content->NodeInfo()->NamespaceEquals(kNameSpaceID_XUL) &&
          content->Tag() != nsGkAtoms::label &&
          content->Tag() != nsGkAtoms::description))) {

      PRBool isWS = PR_TRUE;
      if (mTextLength > 0) {
        const PRUnichar* cp = mText;
        const PRUnichar* end = mText + mTextLength;
        while (cp < end) {
          PRUnichar ch = *cp++;
          if (!XP_IS_SPACE(ch)) {
            isWS = PR_FALSE;
            break;
          }
        }
      }

      if (isWS && mTextLength > 0) {
        mTextLength = 0;
        
        return nsXMLContentSink::FlushText(aReleaseTextNode);
      }
    }
  }

  return nsXMLContentSink::FlushText(aReleaseTextNode);
}

NS_IMETHODIMP
nsXBLContentSink::ReportError(const PRUnichar* aErrorText, 
                              const PRUnichar* aSourceText,
                              nsIScriptError *aError,
                              PRBool *_retval)
{
  NS_PRECONDITION(aError && aSourceText && aErrorText, "Check arguments!!!");

  
  

  
  
  
  

#ifdef DEBUG
  
  fprintf(stderr,
          "\n%s\n%s\n\n",
          NS_LossyConvertUTF16toASCII(aErrorText).get(),
          NS_LossyConvertUTF16toASCII(aSourceText).get());
#endif

  
  
  return nsXMLContentSink::ReportError(aErrorText, 
                                       aSourceText, 
                                       aError,
                                       _retval);
}

nsresult
nsXBLContentSink::ReportUnexpectedElement(nsIAtom* aElementName,
                                          PRUint32 aLineNumber)
{
  
  
  
  mState = eXBL_Error;
  nsAutoString elementName;
  aElementName->ToString(elementName);

  const PRUnichar* params[] = { elementName.get() };

  return nsContentUtils::ReportToConsole(nsContentUtils::eXBL_PROPERTIES,
                                         "UnexpectedElement",
                                         params, NS_ARRAY_LENGTH(params),
                                         mDocumentURI,
                                         EmptyString() ,
                                         aLineNumber, 0 ,
                                         nsIScriptError::errorFlag,
                                         "XBL Content Sink");
}

void
nsXBLContentSink::AddMember(nsXBLProtoImplMember* aMember)
{
  
  if (mImplMember)
    mImplMember->SetNext(aMember); 
  else
    mImplementation->SetMemberList(aMember); 

  mImplMember = aMember; 
}

void
nsXBLContentSink::AddField(nsXBLProtoImplField* aField)
{
  
  if (mImplField)
    mImplField->SetNext(aField); 
  else
    mImplementation->SetFieldList(aField); 

  mImplField = aField; 
}

NS_IMETHODIMP 
nsXBLContentSink::HandleStartElement(const PRUnichar *aName, 
                                     const PRUnichar **aAtts, 
                                     PRUint32 aAttsCount, 
                                     PRInt32 aIndex, 
                                     PRUint32 aLineNumber)
{
  nsresult rv = nsXMLContentSink::HandleStartElement(aName,aAtts,aAttsCount,aIndex,aLineNumber);
  if (NS_FAILED(rv))
    return rv;

  if (mState == eXBL_InBinding && !mBinding) {
    rv = ConstructBinding();
    if (NS_FAILED(rv))
      return rv;
    
    
    
  }

  return rv;
}

NS_IMETHODIMP 
nsXBLContentSink::HandleEndElement(const PRUnichar *aName)
{
  FlushText();

  if (mState != eXBL_InDocument) {
    PRInt32 nameSpaceID;
    nsCOMPtr<nsIAtom> prefix, localName;
    nsContentUtils::SplitExpatName(aName, getter_AddRefs(prefix),
                                   getter_AddRefs(localName), &nameSpaceID);

    if (nameSpaceID == kNameSpaceID_XBL) {
      if (mState == eXBL_Error) {
        
        
        if (!GetCurrentContent()->NodeInfo()->Equals(localName,
                                                     nameSpaceID)) {
          
          
          return NS_OK;
        }
      }
      else if (mState == eXBL_InHandlers) {
        if (localName == nsGkAtoms::handlers) {
          mState = eXBL_InBinding;
          mHandler = nsnull;
        }
        else if (localName == nsGkAtoms::handler)
          mSecondaryState = eXBL_None;
        return NS_OK;
      }
      else if (mState == eXBL_InResources) {
        if (localName == nsGkAtoms::resources)
          mState = eXBL_InBinding;
        return NS_OK;
      }
      else if (mState == eXBL_InImplementation) {
        if (localName == nsGkAtoms::implementation)
          mState = eXBL_InBinding;
        else if (localName == nsGkAtoms::property) {
          mSecondaryState = eXBL_None;
          mProperty = nsnull;
        }
        else if (localName == nsGkAtoms::method) {
          mSecondaryState = eXBL_None;
          mMethod = nsnull;
        }
        else if (localName == nsGkAtoms::field) {
          mSecondaryState = eXBL_None;
          mField = nsnull;
        }
        else if (localName == nsGkAtoms::constructor ||
                 localName == nsGkAtoms::destructor)
          mSecondaryState = eXBL_None;
        else if (localName == nsGkAtoms::getter ||
                 localName == nsGkAtoms::setter)
          mSecondaryState = eXBL_InProperty;
        else if (localName == nsGkAtoms::parameter ||
                 localName == nsGkAtoms::body)
          mSecondaryState = eXBL_InMethod;
        return NS_OK;
      }
      else if (mState == eXBL_InBindings &&
               localName == nsGkAtoms::bindings) {
        mState = eXBL_InDocument;
      }
      
      nsresult rv = nsXMLContentSink::HandleEndElement(aName);
      if (NS_FAILED(rv))
        return rv;

      if (mState == eXBL_InBinding && localName == nsGkAtoms::binding) {
        mState = eXBL_InBindings;
        if (mBinding) {  
          mBinding->Initialize();
          mBinding = nsnull; 
        }
      }

      return NS_OK;
    }
  }

  return nsXMLContentSink::HandleEndElement(aName);
}

NS_IMETHODIMP 
nsXBLContentSink::HandleCDataSection(const PRUnichar *aData, 
                                     PRUint32 aLength)
{
  if (mState == eXBL_InHandlers || mState == eXBL_InImplementation)
    return AddText(aData, aLength);
  return nsXMLContentSink::HandleCDataSection(aData, aLength);
}

#define ENSURE_XBL_STATE(_cond)                                                       \
  PR_BEGIN_MACRO                                                                      \
    if (!(_cond)) { ReportUnexpectedElement(aTagName, aLineNumber); return PR_TRUE; } \
  PR_END_MACRO

PRBool 
nsXBLContentSink::OnOpenContainer(const PRUnichar **aAtts, 
                                  PRUint32 aAttsCount, 
                                  PRInt32 aNameSpaceID, 
                                  nsIAtom* aTagName,
                                  PRUint32 aLineNumber)
{
  if (mState == eXBL_Error) {
    return PR_TRUE;
  }
  
  if (aNameSpaceID != kNameSpaceID_XBL) {
    
    return PR_TRUE;
  }

  PRBool ret = PR_TRUE;
  if (aTagName == nsGkAtoms::bindings) {
    ENSURE_XBL_STATE(mState == eXBL_InDocument);
      
    NS_NewXBLDocumentInfo(mDocument, &mDocInfo);
    if (!mDocInfo) {
      mState = eXBL_Error;
      return PR_TRUE;
    }

    mDocument->BindingManager()->PutXBLDocumentInfo(mDocInfo);

    nsIURI *uri = mDocument->GetDocumentURI();
      
    PRBool isChrome = PR_FALSE;
    PRBool isRes = PR_FALSE;

    uri->SchemeIs("chrome", &isChrome);
    uri->SchemeIs("resource", &isRes);
    mIsChromeOrResource = isChrome || isRes;
      
    nsIXBLDocumentInfo* info = mDocInfo;
    NS_RELEASE(info); 
    mState = eXBL_InBindings;
  }
  else if (aTagName == nsGkAtoms::binding) {
    ENSURE_XBL_STATE(mState == eXBL_InBindings);
    mState = eXBL_InBinding;
  }
  else if (aTagName == nsGkAtoms::handlers) {
    ENSURE_XBL_STATE(mState == eXBL_InBinding && mBinding);
    mState = eXBL_InHandlers;
    ret = PR_FALSE;
  }
  else if (aTagName == nsGkAtoms::handler) {
    ENSURE_XBL_STATE(mState == eXBL_InHandlers);
    mSecondaryState = eXBL_InHandler;
    ConstructHandler(aAtts, aLineNumber);
    ret = PR_FALSE;
  }
  else if (aTagName == nsGkAtoms::resources) {
    ENSURE_XBL_STATE(mState == eXBL_InBinding && mBinding);
    mState = eXBL_InResources;
    
    
  }
  else if (aTagName == nsGkAtoms::stylesheet || aTagName == nsGkAtoms::image) {
    ENSURE_XBL_STATE(mState == eXBL_InResources);
    NS_ASSERTION(mBinding, "Must have binding here");
    ConstructResource(aAtts, aTagName);
  }
  else if (aTagName == nsGkAtoms::implementation) {
    ENSURE_XBL_STATE(mState == eXBL_InBinding && mBinding);
    mState = eXBL_InImplementation;
    ConstructImplementation(aAtts);
    
    
  }
  else if (aTagName == nsGkAtoms::constructor) {
    ENSURE_XBL_STATE(mState == eXBL_InImplementation &&
                     mSecondaryState == eXBL_None);
    NS_ASSERTION(mBinding, "Must have binding here");
      
    mSecondaryState = eXBL_InConstructor;
    nsXBLProtoImplAnonymousMethod* newMethod =
      new nsXBLProtoImplAnonymousMethod();
    if (newMethod) {
      newMethod->SetLineNumber(aLineNumber);
      mBinding->SetConstructor(newMethod);
      AddMember(newMethod);
    }
  }
  else if (aTagName == nsGkAtoms::destructor) {
    ENSURE_XBL_STATE(mState == eXBL_InImplementation &&
                     mSecondaryState == eXBL_None);
    NS_ASSERTION(mBinding, "Must have binding here");
    mSecondaryState = eXBL_InDestructor;
    nsXBLProtoImplAnonymousMethod* newMethod =
      new nsXBLProtoImplAnonymousMethod();
    if (newMethod) {
      newMethod->SetLineNumber(aLineNumber);
      mBinding->SetDestructor(newMethod);
      AddMember(newMethod);
    }
  }
  else if (aTagName == nsGkAtoms::field) {
    ENSURE_XBL_STATE(mState == eXBL_InImplementation &&
                     mSecondaryState == eXBL_None);
    NS_ASSERTION(mBinding, "Must have binding here");
    mSecondaryState = eXBL_InField;
    ConstructField(aAtts, aLineNumber);
  }
  else if (aTagName == nsGkAtoms::property) {
    ENSURE_XBL_STATE(mState == eXBL_InImplementation &&
                     mSecondaryState == eXBL_None);
    NS_ASSERTION(mBinding, "Must have binding here");
    mSecondaryState = eXBL_InProperty;
    ConstructProperty(aAtts);
  }
  else if (aTagName == nsGkAtoms::getter) {
    ENSURE_XBL_STATE(mSecondaryState == eXBL_InProperty && mProperty);
    NS_ASSERTION(mState == eXBL_InImplementation, "Unexpected state");
    mProperty->SetGetterLineNumber(aLineNumber);
    mSecondaryState = eXBL_InGetter;
  }
  else if (aTagName == nsGkAtoms::setter) {
    ENSURE_XBL_STATE(mSecondaryState == eXBL_InProperty && mProperty);
    NS_ASSERTION(mState == eXBL_InImplementation, "Unexpected state");
    mProperty->SetSetterLineNumber(aLineNumber);
    mSecondaryState = eXBL_InSetter;
  }
  else if (aTagName == nsGkAtoms::method) {
    ENSURE_XBL_STATE(mState == eXBL_InImplementation &&
                     mSecondaryState == eXBL_None);
    NS_ASSERTION(mBinding, "Must have binding here");
    mSecondaryState = eXBL_InMethod;
    ConstructMethod(aAtts);
  }
  else if (aTagName == nsGkAtoms::parameter) {
    ENSURE_XBL_STATE(mSecondaryState == eXBL_InMethod && mMethod);
    NS_ASSERTION(mState == eXBL_InImplementation, "Unexpected state");
    ConstructParameter(aAtts);
  }
  else if (aTagName == nsGkAtoms::body) {
    ENSURE_XBL_STATE(mSecondaryState == eXBL_InMethod && mMethod);
    NS_ASSERTION(mState == eXBL_InImplementation, "Unexpected state");
    
    mMethod->SetLineNumber(aLineNumber);
    mSecondaryState = eXBL_InBody;
  }

  return ret && mState != eXBL_InResources && mState != eXBL_InImplementation;
}

#undef ENSURE_XBL_STATE

nsresult
nsXBLContentSink::ConstructBinding()
{
  nsCOMPtr<nsIContent> binding = GetCurrentContent();
  nsAutoString id;
  binding->GetAttr(kNameSpaceID_None, nsGkAtoms::id, id);
  NS_ConvertUTF16toUTF8 cid(id);

  nsresult rv = NS_OK;
  
  if (!cid.IsEmpty()) {
    mBinding = new nsXBLPrototypeBinding();
    if (!mBinding)
      return NS_ERROR_OUT_OF_MEMORY;
      
    rv = mBinding->Init(cid, mDocInfo, binding);
    if (NS_SUCCEEDED(rv) &&
        NS_SUCCEEDED(mDocInfo->SetPrototypeBinding(cid, mBinding))) {
      if (!mFoundFirstBinding) {
        mFoundFirstBinding = PR_TRUE;
        mDocInfo->SetFirstPrototypeBinding(mBinding);
      }
      binding->UnsetAttr(kNameSpaceID_None, nsGkAtoms::id, PR_FALSE);
    } else {
      delete mBinding;
      mBinding = nsnull;
    }
  }

  return rv;
}

static PRBool
FindValue(const PRUnichar **aAtts, nsIAtom *aAtom, const PRUnichar **aResult)
{
  nsCOMPtr<nsIAtom> prefix, localName;
  for (; *aAtts; aAtts += 2) {
    PRInt32 nameSpaceID;
    nsContentUtils::SplitExpatName(aAtts[0], getter_AddRefs(prefix),
                                   getter_AddRefs(localName), &nameSpaceID);

    
    if (nameSpaceID == kNameSpaceID_None && localName == aAtom) {
      *aResult = aAtts[1];

      return PR_TRUE;
    }
  }

  return PR_FALSE;
}

void
nsXBLContentSink::ConstructHandler(const PRUnichar **aAtts, PRUint32 aLineNumber)
{
  const PRUnichar* event          = nsnull;
  const PRUnichar* modifiers      = nsnull;
  const PRUnichar* button         = nsnull;
  const PRUnichar* clickcount     = nsnull;
  const PRUnichar* keycode        = nsnull;
  const PRUnichar* charcode       = nsnull;
  const PRUnichar* phase          = nsnull;
  const PRUnichar* command        = nsnull;
  const PRUnichar* action         = nsnull;
  const PRUnichar* group          = nsnull;
  const PRUnichar* preventdefault = nsnull;
  const PRUnichar* allowuntrusted = nsnull;

  nsCOMPtr<nsIAtom> prefix, localName;
  for (; *aAtts; aAtts += 2) {
    PRInt32 nameSpaceID;
    nsContentUtils::SplitExpatName(aAtts[0], getter_AddRefs(prefix),
                                   getter_AddRefs(localName), &nameSpaceID);

    if (nameSpaceID != kNameSpaceID_None) {
      continue;
    }

    
    if (localName == nsGkAtoms::event)
      event = aAtts[1];
    else if (localName == nsGkAtoms::modifiers)
      modifiers = aAtts[1];
    else if (localName == nsGkAtoms::button)
      button = aAtts[1];
    else if (localName == nsGkAtoms::clickcount)
      clickcount = aAtts[1];
    else if (localName == nsGkAtoms::keycode)
      keycode = aAtts[1];
    else if (localName == nsGkAtoms::key || localName == nsGkAtoms::charcode)
      charcode = aAtts[1];
    else if (localName == nsGkAtoms::phase)
      phase = aAtts[1];
    else if (localName == nsGkAtoms::command)
      command = aAtts[1];
    else if (localName == nsGkAtoms::action)
      action = aAtts[1];
    else if (localName == nsGkAtoms::group)
      group = aAtts[1];
    else if (localName == nsGkAtoms::preventdefault)
      preventdefault = aAtts[1];
    else if (localName == nsGkAtoms::allowuntrusted)
      allowuntrusted = aAtts[1];
  }

  if (command && !mIsChromeOrResource) {
    
    
    mState = eXBL_Error;
    nsContentUtils::ReportToConsole(nsContentUtils::eXBL_PROPERTIES,
                                    "CommandNotInChrome", nsnull, 0,
                                    mDocumentURI,
                                    EmptyString() ,
                                    aLineNumber, 0 ,
                                    nsIScriptError::errorFlag,
                                    "XBL Content Sink");
    return; 
  }

  
  
  nsXBLPrototypeHandler* newHandler;
  newHandler = new nsXBLPrototypeHandler(event, phase, action, command,
                                         keycode, charcode, modifiers, button,
                                         clickcount, group, preventdefault,
                                         allowuntrusted, mBinding, aLineNumber);

  if (newHandler) {
    
    if (mHandler) {
      
      mHandler->SetNextHandler(newHandler);
    }
    else {
      
      mBinding->SetPrototypeHandlers(newHandler);
    }
    
    
    mHandler = newHandler;
  } else {
    mState = eXBL_Error;
  }
}

void
nsXBLContentSink::ConstructResource(const PRUnichar **aAtts,
                                    nsIAtom* aResourceType)
{
  if (!mBinding)
    return;

  const PRUnichar* src = nsnull;
  if (FindValue(aAtts, nsGkAtoms::src, &src)) {
    mBinding->AddResource(aResourceType, nsDependentString(src));
  }
}

void
nsXBLContentSink::ConstructImplementation(const PRUnichar **aAtts)
{
  mImplementation = nsnull;
  mImplMember = nsnull;
  mImplField = nsnull;
  
  if (!mBinding)
    return;

  const PRUnichar* name = nsnull;

  nsCOMPtr<nsIAtom> prefix, localName;
  for (; *aAtts; aAtts += 2) {
    PRInt32 nameSpaceID;
    nsContentUtils::SplitExpatName(aAtts[0], getter_AddRefs(prefix),
                                   getter_AddRefs(localName), &nameSpaceID);

    if (nameSpaceID != kNameSpaceID_None) {
      continue;
    }

    
    if (localName == nsGkAtoms::name) {
      name = aAtts[1];
    }
    else if (localName == nsGkAtoms::implements) {
      
      
      
      
      
      
      
      
      
      PRBool hasUniversalXPConnect;
      nsresult rv = mDocument->NodePrincipal()->
        IsCapabilityEnabled("UniversalXPConnect", nsnull,
                            &hasUniversalXPConnect);
      if (NS_SUCCEEDED(rv) && hasUniversalXPConnect) {
        mBinding->ConstructInterfaceTable(nsDependentString(aAtts[1]));
      }
    }
  }

  NS_NewXBLProtoImpl(mBinding, name, &mImplementation);
}

void
nsXBLContentSink::ConstructField(const PRUnichar **aAtts, PRUint32 aLineNumber)
{
  const PRUnichar* name     = nsnull;
  const PRUnichar* readonly = nsnull;

  nsCOMPtr<nsIAtom> prefix, localName;
  for (; *aAtts; aAtts += 2) {
    PRInt32 nameSpaceID;
    nsContentUtils::SplitExpatName(aAtts[0], getter_AddRefs(prefix),
                                   getter_AddRefs(localName), &nameSpaceID);

    if (nameSpaceID != kNameSpaceID_None) {
      continue;
    }

    
    if (localName == nsGkAtoms::name) {
      name = aAtts[1];
    }
    else if (localName == nsGkAtoms::readonly) {
      readonly = aAtts[1];
    }
  }

  if (name) {
    
    
    mField = new nsXBLProtoImplField(name, readonly);
    if (mField) {
      mField->SetLineNumber(aLineNumber);
      AddField(mField);
    }
  }
}

void
nsXBLContentSink::ConstructProperty(const PRUnichar **aAtts)
{
  const PRUnichar* name     = nsnull;
  const PRUnichar* readonly = nsnull;
  const PRUnichar* onget    = nsnull;
  const PRUnichar* onset    = nsnull;

  nsCOMPtr<nsIAtom> prefix, localName;
  for (; *aAtts; aAtts += 2) {
    PRInt32 nameSpaceID;
    nsContentUtils::SplitExpatName(aAtts[0], getter_AddRefs(prefix),
                                   getter_AddRefs(localName), &nameSpaceID);

    if (nameSpaceID != kNameSpaceID_None) {
      continue;
    }

    
    if (localName == nsGkAtoms::name) {
      name = aAtts[1];
    }
    else if (localName == nsGkAtoms::readonly) {
      readonly = aAtts[1];
    }
    else if (localName == nsGkAtoms::onget) {
      onget = aAtts[1];
    }
    else if (localName == nsGkAtoms::onset) {
      onset = aAtts[1];
    }
  }

  if (name) {
    
    
    mProperty = new nsXBLProtoImplProperty(name, onget, onset, readonly);
    if (mProperty) {
      AddMember(mProperty);
    }
  }
}

void
nsXBLContentSink::ConstructMethod(const PRUnichar **aAtts)
{
  mMethod = nsnull;

  const PRUnichar* name = nsnull;
  if (FindValue(aAtts, nsGkAtoms::name, &name)) {
    mMethod = new nsXBLProtoImplMethod(name);
  }

  if (mMethod) {
    AddMember(mMethod);
  }
}

void
nsXBLContentSink::ConstructParameter(const PRUnichar **aAtts)
{
  if (!mMethod)
    return;

  const PRUnichar* name = nsnull;
  if (FindValue(aAtts, nsGkAtoms::name, &name)) {
    mMethod->AddParameter(nsDependentString(name));
  }
}

nsresult
nsXBLContentSink::CreateElement(const PRUnichar** aAtts, PRUint32 aAttsCount,
                                nsINodeInfo* aNodeInfo, PRUint32 aLineNumber,
                                nsIContent** aResult, PRBool* aAppendContent,
                                PRBool aFromParser)
{
#ifdef MOZ_XUL
  if (!aNodeInfo->NamespaceEquals(kNameSpaceID_XUL)) {
#endif
    return nsXMLContentSink::CreateElement(aAtts, aAttsCount, aNodeInfo,
                                           aLineNumber, aResult,
                                           aAppendContent, aFromParser);
#ifdef MOZ_XUL
  }

  *aAppendContent = PR_TRUE;
  nsRefPtr<nsXULPrototypeElement> prototype = new nsXULPrototypeElement();
  if (!prototype)
    return NS_ERROR_OUT_OF_MEMORY;

  prototype->mNodeInfo = aNodeInfo;
  
  
  prototype->mScriptTypeID = nsIProgrammingLanguage::JAVASCRIPT;

  AddAttributesToXULPrototype(aAtts, aAttsCount, prototype);

  return nsXULElement::Create(prototype, mDocument, PR_FALSE, aResult);
#endif
}

nsresult 
nsXBLContentSink::AddAttributes(const PRUnichar** aAtts,
                                nsIContent* aContent)
{
  if (aContent->IsNodeOfType(nsINode::eXUL))
    return NS_OK; 

  return nsXMLContentSink::AddAttributes(aAtts, aContent);
}

#ifdef MOZ_XUL
nsresult
nsXBLContentSink::AddAttributesToXULPrototype(const PRUnichar **aAtts, 
                                              PRUint32 aAttsCount, 
                                              nsXULPrototypeElement* aElement)
{
  
  nsresult rv;

  
  nsXULPrototypeAttribute* attrs = nsnull;
  if (aAttsCount > 0) {
    attrs = new nsXULPrototypeAttribute[aAttsCount];
    if (!attrs)
      return NS_ERROR_OUT_OF_MEMORY;
  }

  aElement->mAttributes    = attrs;
  aElement->mNumAttributes = aAttsCount;

  
  nsCOMPtr<nsIAtom> prefix, localName;

  PRUint32 i;  
  for (i = 0; i < aAttsCount; ++i) {
    PRInt32 nameSpaceID;
    nsContentUtils::SplitExpatName(aAtts[i * 2], getter_AddRefs(prefix),
                                   getter_AddRefs(localName), &nameSpaceID);

    if (nameSpaceID == kNameSpaceID_None) {
      attrs[i].mName.SetTo(localName);
    }
    else {
      nsCOMPtr<nsINodeInfo> ni;
      ni = mNodeInfoManager->GetNodeInfo(localName, prefix, nameSpaceID);
      attrs[i].mName.SetTo(ni);
    }
    
    rv = aElement->SetAttrAt(i, nsDependentString(aAtts[i * 2 + 1]),
                             mDocumentURI); 
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}
#endif
