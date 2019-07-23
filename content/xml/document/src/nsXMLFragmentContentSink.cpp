





































#include "nsCOMPtr.h"
#include "nsXMLContentSink.h"
#include "nsIFragmentContentSink.h"
#include "nsIXMLContentSink.h"
#include "nsContentSink.h"
#include "nsIExpatSink.h"
#include "nsIDTD.h"
#include "nsIParser.h"
#include "nsIDocument.h"
#include "nsIDOMDocumentFragment.h"
#include "nsIContent.h"
#include "nsGkAtoms.h"
#include "nsINodeInfo.h"
#include "nsNodeInfoManager.h"
#include "nsContentCreatorFunctions.h"
#include "nsDOMError.h"
#include "nsIConsoleService.h"
#include "nsIScriptError.h"
#include "nsServiceManagerUtils.h"
#include "nsContentUtils.h"
#include "nsIScriptSecurityManager.h"
#include "nsNetUtil.h"
#include "nsTHashtable.h"
#include "nsHashKeys.h"
#include "nsTArray.h"
#include "nsCycleCollectionParticipant.h"

class nsXMLFragmentContentSink : public nsXMLContentSink,
                                 public nsIFragmentContentSink
{
public:
  nsXMLFragmentContentSink(PRBool aAllContent = PR_FALSE);
  virtual ~nsXMLFragmentContentSink();

  NS_DECL_AND_IMPL_ZEROING_OPERATOR_NEW

  
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED_NO_UNLINK(nsXMLFragmentContentSink,
                                                     nsXMLContentSink)

  
  NS_IMETHOD HandleDoctypeDecl(const nsAString & aSubset, 
                               const nsAString & aName, 
                               const nsAString & aSystemId, 
                               const nsAString & aPublicId,
                               nsISupports* aCatalogData);
  NS_IMETHOD HandleProcessingInstruction(const PRUnichar *aTarget, 
                                         const PRUnichar *aData);
  NS_IMETHOD HandleXMLDeclaration(const PRUnichar *aVersion,
                                  const PRUnichar *aEncoding,
                                  PRInt32 aStandalone);
  NS_IMETHOD ReportError(const PRUnichar* aErrorText, 
                         const PRUnichar* aSourceText,
                         nsIScriptError *aError,
                         PRBool *_retval);

  
  NS_IMETHOD WillBuildModel(nsDTDMode aDTDMode);
  NS_IMETHOD DidBuildModel(PRBool aTerminated);
  NS_IMETHOD SetDocumentCharset(nsACString& aCharset);
  virtual nsISupports *GetTarget();
  NS_IMETHOD DidProcessATokenImpl();

  

  
  NS_IMETHOD GetFragment(PRBool aWillOwnFragment,
                         nsIDOMDocumentFragment** aFragment);
  NS_IMETHOD SetTargetDocument(nsIDocument* aDocument);
  NS_IMETHOD WillBuildContent();
  NS_IMETHOD DidBuildContent();
  NS_IMETHOD IgnoreFirstContainer();

protected:
  virtual PRBool SetDocElement(PRInt32 aNameSpaceID, 
                               nsIAtom *aTagName,
                               nsIContent *aContent);
  virtual nsresult CreateElement(const PRUnichar** aAtts, PRUint32 aAttsCount,
                                 nsINodeInfo* aNodeInfo, PRUint32 aLineNumber,
                                 nsIContent** aResult, PRBool* aAppendContent,
                                 PRBool aFromParser);
  virtual nsresult CloseElement(nsIContent* aContent);

  virtual void MaybeStartLayout(PRBool aIgnorePendingSheets);

  
  virtual nsresult ProcessStyleLink(nsIContent* aElement,
                                    const nsSubstring& aHref,
                                    PRBool aAlternate,
                                    const nsSubstring& aTitle,
                                    const nsSubstring& aType,
                                    const nsSubstring& aMedia);
  nsresult LoadXSLStyleSheet(nsIURI* aUrl);
  void StartLayout();

  nsCOMPtr<nsIDocument> mTargetDocument;
  
  nsCOMPtr<nsIContent>  mRoot;
  PRPackedBool          mParseError;

  
  PRPackedBool          mAllContent;
};

static nsresult
NewXMLFragmentContentSinkHelper(PRBool aAllContent, nsIFragmentContentSink** aResult)
{
  nsXMLFragmentContentSink* it = new nsXMLFragmentContentSink(aAllContent);
  if (!it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  
  NS_ADDREF(*aResult = it);
  
  return NS_OK;
}

nsresult
NS_NewXMLFragmentContentSink2(nsIFragmentContentSink** aResult)
{
  return NewXMLFragmentContentSinkHelper(PR_TRUE, aResult);
}

nsresult
NS_NewXMLFragmentContentSink(nsIFragmentContentSink** aResult)
{
  return NewXMLFragmentContentSinkHelper(PR_FALSE, aResult);
}

nsXMLFragmentContentSink::nsXMLFragmentContentSink(PRBool aAllContent)
 : mParseError(PR_FALSE), mAllContent(aAllContent)
{
}

nsXMLFragmentContentSink::~nsXMLFragmentContentSink()
{
}

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(nsXMLFragmentContentSink)
  NS_INTERFACE_MAP_ENTRY(nsIFragmentContentSink)
NS_INTERFACE_MAP_END_INHERITING(nsXMLContentSink)

NS_IMPL_ADDREF_INHERITED(nsXMLFragmentContentSink, nsXMLContentSink)
NS_IMPL_RELEASE_INHERITED(nsXMLFragmentContentSink, nsXMLContentSink)

NS_IMPL_CYCLE_COLLECTION_CLASS(nsXMLFragmentContentSink)

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(nsXMLFragmentContentSink,
                                                  nsXMLContentSink)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mTargetDocument)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mRoot)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMETHODIMP 
nsXMLFragmentContentSink::WillBuildModel(nsDTDMode aDTDMode)
{
  if (mRoot) {
    return NS_OK;
  }

  mState = eXMLContentSinkState_InDocumentElement;

  NS_ASSERTION(mTargetDocument, "Need a document!");

  nsCOMPtr<nsIDOMDocumentFragment> frag;
  nsresult rv = NS_NewDocumentFragment(getter_AddRefs(frag), mNodeInfoManager);
  NS_ENSURE_SUCCESS(rv, rv);

  mRoot = do_QueryInterface(frag);
  
  if (mAllContent) {
    
    PushContent(mRoot);
  }

  return rv;
}

NS_IMETHODIMP 
nsXMLFragmentContentSink::DidBuildModel(PRBool aTerminated)
{
  if (mAllContent) {
    PopContent();  
  }

  nsCOMPtr<nsIParser> kungFuDeathGrip(mParser);

  
  
  mParser = nsnull;

  return NS_OK;
}

NS_IMETHODIMP 
nsXMLFragmentContentSink::SetDocumentCharset(nsACString& aCharset)
{
  NS_NOTREACHED("fragments shouldn't set charset");
  return NS_OK;
}

nsISupports *
nsXMLFragmentContentSink::GetTarget()
{
  return mTargetDocument;
}



PRBool
nsXMLFragmentContentSink::SetDocElement(PRInt32 aNameSpaceID,
                                        nsIAtom* aTagName,
                                        nsIContent *aContent)
{
  
  return PR_FALSE;
}

nsresult
nsXMLFragmentContentSink::CreateElement(const PRUnichar** aAtts, PRUint32 aAttsCount,
                                        nsINodeInfo* aNodeInfo, PRUint32 aLineNumber,
                                        nsIContent** aResult, PRBool* aAppendContent,
                                        PRBool aFromParser)
{
  
  
  nsresult rv = nsXMLContentSink::CreateElement(aAtts, aAttsCount,
                                                aNodeInfo, aLineNumber,
                                                aResult, aAppendContent,
                                                PR_FALSE);

  
  
  
  if (!mAllContent && mContentStack.Length() == 0) {
    *aAppendContent = PR_FALSE;
  }

  return rv;
}

nsresult
nsXMLFragmentContentSink::CloseElement(nsIContent* aContent)
{
  
  return NS_OK;
}

void
nsXMLFragmentContentSink::MaybeStartLayout(PRBool aIgnorePendingSheets)
{
  return;
}



NS_IMETHODIMP
nsXMLFragmentContentSink::HandleDoctypeDecl(const nsAString & aSubset, 
                                            const nsAString & aName, 
                                            const nsAString & aSystemId, 
                                            const nsAString & aPublicId,
                                            nsISupports* aCatalogData)
{
  NS_NOTREACHED("fragments shouldn't have doctype declarations");

  return NS_OK;
}

NS_IMETHODIMP
nsXMLFragmentContentSink::HandleProcessingInstruction(const PRUnichar *aTarget, 
                                                      const PRUnichar *aData)
{
  FlushText();

  nsresult result = NS_OK;
  const nsDependentString target(aTarget);
  const nsDependentString data(aData);

  nsCOMPtr<nsIContent> node;

  result = NS_NewXMLProcessingInstruction(getter_AddRefs(node),
                                          mNodeInfoManager, target, data);
  if (NS_SUCCEEDED(result)) {
    
    result = AddContentAsLeaf(node);
  }
  return result;
}

NS_IMETHODIMP
nsXMLFragmentContentSink::HandleXMLDeclaration(const PRUnichar *aVersion,
                                               const PRUnichar *aEncoding,
                                               PRInt32 aStandalone)
{
  NS_NOTREACHED("fragments shouldn't have XML declarations");
  return NS_OK;
}

NS_IMETHODIMP
nsXMLFragmentContentSink::ReportError(const PRUnichar* aErrorText, 
                                      const PRUnichar* aSourceText,
                                      nsIScriptError *aError,
                                      PRBool *_retval)
{
  NS_PRECONDITION(aError && aSourceText && aErrorText, "Check arguments!!!");

  
  *_retval = PR_TRUE;

  mParseError = PR_TRUE;

#ifdef DEBUG
  
  fprintf(stderr,
          "\n%s\n%s\n\n",
          NS_LossyConvertUTF16toASCII(aErrorText).get(),
          NS_LossyConvertUTF16toASCII(aSourceText).get());
#endif

  
  mState = eXMLContentSinkState_InProlog;

  
  nsCOMPtr<nsIDOMNode> node(do_QueryInterface(mRoot));
  if (node) {
    for (;;) {
      nsCOMPtr<nsIDOMNode> child, dummy;
      node->GetLastChild(getter_AddRefs(child));
      if (!child)
        break;
      node->RemoveChild(child, getter_AddRefs(dummy));
    }
  }

  
  
  
  mTextLength = 0;

  return NS_OK; 
}

nsresult
nsXMLFragmentContentSink::ProcessStyleLink(nsIContent* aElement,
                                           const nsSubstring& aHref,
                                           PRBool aAlternate,
                                           const nsSubstring& aTitle,
                                           const nsSubstring& aType,
                                           const nsSubstring& aMedia)
{
  
  return NS_OK;
}

nsresult
nsXMLFragmentContentSink::LoadXSLStyleSheet(nsIURI* aUrl)
{
  NS_NOTREACHED("fragments shouldn't have XSL style sheets");
  return NS_ERROR_UNEXPECTED;
}

void
nsXMLFragmentContentSink::StartLayout()
{
  NS_NOTREACHED("fragments shouldn't layout");
}



NS_IMETHODIMP 
nsXMLFragmentContentSink::GetFragment(PRBool aWillOwnFragment,
                                      nsIDOMDocumentFragment** aFragment)
{
  *aFragment = nsnull;
  if (mParseError) {
    
    return NS_ERROR_DOM_SYNTAX_ERR;
  } else if (mRoot) {
    nsresult rv = CallQueryInterface(mRoot, aFragment);
    if (NS_SUCCEEDED(rv) && aWillOwnFragment) {
      mRoot = nsnull;
    }
    return rv;
  } else {
    return NS_OK;
  }
}

NS_IMETHODIMP
nsXMLFragmentContentSink::SetTargetDocument(nsIDocument* aTargetDocument)
{
  NS_ENSURE_ARG_POINTER(aTargetDocument);

  mTargetDocument = aTargetDocument;
  mNodeInfoManager = aTargetDocument->NodeInfoManager();

  return NS_OK;
}

NS_IMETHODIMP
nsXMLFragmentContentSink::WillBuildContent()
{
  
  
  if (!mAllContent) {
    PushContent(mRoot);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsXMLFragmentContentSink::DidBuildContent()
{
  
  if (!mAllContent) {
    
    
    if (!mParseError) {
      FlushText();
    }
    PopContent();
  }

  return NS_OK;
}

NS_IMETHODIMP
nsXMLFragmentContentSink::DidProcessATokenImpl()
{
  return NS_OK;
}

NS_IMETHODIMP
nsXMLFragmentContentSink::IgnoreFirstContainer()
{
  NS_NOTREACHED("XML isn't as broken as HTML");
  return NS_ERROR_FAILURE;
}







class nsXHTMLParanoidFragmentSink : public nsXMLFragmentContentSink
{
public:
  nsXHTMLParanoidFragmentSink();

  static nsresult Init();
  static void Cleanup();

  
  NS_DECL_ISUPPORTS_INHERITED
  
  
  nsresult AddAttributes(const PRUnichar** aNode, nsIContent* aContent);

  
  NS_IMETHOD HandleStartElement(const PRUnichar *aName,
                                const PRUnichar **aAtts,
                                PRUint32 aAttsCount, PRInt32 aIndex,
                                PRUint32 aLineNumber);
    
  NS_IMETHOD HandleEndElement(const PRUnichar *aName);

  NS_IMETHOD HandleComment(const PRUnichar *aName);

  NS_IMETHOD HandleProcessingInstruction(const PRUnichar *aTarget, 
                                         const PRUnichar *aData);

  NS_IMETHOD HandleCDataSection(const PRUnichar *aData, 
                                PRUint32 aLength);

  NS_IMETHOD HandleCharacterData(const PRUnichar *aData,
                                 PRUint32 aLength);
protected:
  PRUint32 mSkipLevel; 
  
  static nsTHashtable<nsISupportsHashKey>* sAllowedTags;
  static nsTHashtable<nsISupportsHashKey>* sAllowedAttributes;
};

nsTHashtable<nsISupportsHashKey>* nsXHTMLParanoidFragmentSink::sAllowedTags;
nsTHashtable<nsISupportsHashKey>* nsXHTMLParanoidFragmentSink::sAllowedAttributes;

nsXHTMLParanoidFragmentSink::nsXHTMLParanoidFragmentSink():
  nsXMLFragmentContentSink(PR_FALSE), mSkipLevel(0)
{
}

nsresult
nsXHTMLParanoidFragmentSink::Init()
{
  nsresult rv = NS_ERROR_FAILURE;
  
  if (sAllowedTags) {
    return NS_OK;
  }

  sAllowedTags = new nsTHashtable<nsISupportsHashKey>();
  if (sAllowedTags) {
    rv = sAllowedTags->Init(80);
    for (PRUint32 i = 0; kDefaultAllowedTags[i] && NS_SUCCEEDED(rv); i++) {
      if (!sAllowedTags->PutEntry(*kDefaultAllowedTags[i])) {
        rv = NS_ERROR_OUT_OF_MEMORY;
      }
    }
  }

  sAllowedAttributes = new nsTHashtable<nsISupportsHashKey>();
  if (sAllowedAttributes && NS_SUCCEEDED(rv)) {
    rv = sAllowedAttributes->Init(80);
    for (PRUint32 i = 0;
         kDefaultAllowedAttributes[i] && NS_SUCCEEDED(rv); i++) {
      if (!sAllowedAttributes->PutEntry(*kDefaultAllowedAttributes[i])) {
        rv = NS_ERROR_OUT_OF_MEMORY;
      }
    }
  }

  if (NS_FAILED(rv)) {
    NS_WARNING("Failed to populate whitelist hash sets");
    Cleanup();
  }

  return rv;
}

void
nsXHTMLParanoidFragmentSink::Cleanup()
{
  if (sAllowedTags) {
    delete sAllowedTags;
    sAllowedTags = nsnull;
  }
  
  if (sAllowedAttributes) {
    delete sAllowedAttributes;
    sAllowedAttributes = nsnull;
  }
}

nsresult
NS_NewXHTMLParanoidFragmentSink(nsIFragmentContentSink** aResult)
{
  nsXHTMLParanoidFragmentSink* it = new nsXHTMLParanoidFragmentSink();
  if (!it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  nsresult rv = nsXHTMLParanoidFragmentSink::Init();
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ADDREF(*aResult = it);
  
  return NS_OK;
}

void
NS_XHTMLParanoidFragmentSinkShutdown()
{
  nsXHTMLParanoidFragmentSink::Cleanup();
}

NS_IMPL_ISUPPORTS_INHERITED0(nsXHTMLParanoidFragmentSink,
                             nsXMLFragmentContentSink)

nsresult
nsXHTMLParanoidFragmentSink::AddAttributes(const PRUnichar** aAtts,
                                           nsIContent* aContent)
{
  nsresult rv;

  
  nsIScriptSecurityManager* secMan = nsContentUtils::GetSecurityManager();
  nsCOMPtr<nsIURI> baseURI;
  PRUint32 flags = nsIScriptSecurityManager::DISALLOW_INHERIT_PRINCIPAL;

  
  
  
  
  nsTArray<const PRUnichar *> allowedAttrs;
  PRInt32 nameSpaceID;
  nsCOMPtr<nsIAtom> prefix, localName;
  nsCOMPtr<nsINodeInfo> nodeInfo;
  while (*aAtts) {
    nsContentUtils::SplitExpatName(aAtts[0], getter_AddRefs(prefix),
                                   getter_AddRefs(localName), &nameSpaceID);
    nodeInfo = mNodeInfoManager->GetNodeInfo(localName, prefix, nameSpaceID);
    NS_ENSURE_TRUE(nodeInfo, NS_ERROR_OUT_OF_MEMORY);
    
    if (IsAttrURI(nodeInfo->NameAtom())) {
      if (!aAtts[1])
        rv = NS_ERROR_FAILURE;
      if (!baseURI)
        baseURI = aContent->GetBaseURI();
      nsCOMPtr<nsIURI> attrURI;
      rv = NS_NewURI(getter_AddRefs(attrURI), nsDependentString(aAtts[1]),
                     nsnull, baseURI);
      if (NS_SUCCEEDED(rv)) {
        rv = secMan->CheckLoadURIWithPrincipal(mTargetDocument->NodePrincipal(),
                                               attrURI, flags);
      }
    }

    if (NS_SUCCEEDED(rv)) {
      allowedAttrs.AppendElement(aAtts[0]);
      allowedAttrs.AppendElement(aAtts[1]);
    }

    aAtts += 2;
  }
  allowedAttrs.AppendElement((const PRUnichar*) nsnull);

  return nsXMLFragmentContentSink::AddAttributes(allowedAttrs.Elements(),
                                                 aContent);
}

NS_IMETHODIMP
nsXHTMLParanoidFragmentSink::HandleStartElement(const PRUnichar *aName,
                                                const PRUnichar **aAtts,
                                                PRUint32 aAttsCount,
                                                PRInt32 aIndex,
                                                PRUint32 aLineNumber)
{
  PRInt32 nameSpaceID;
  nsCOMPtr<nsIAtom> prefix, localName;
  nsContentUtils::SplitExpatName(aName, getter_AddRefs(prefix),
                                 getter_AddRefs(localName), &nameSpaceID);
  
  
  if (nameSpaceID != kNameSpaceID_XHTML)
    return NS_OK;
  
  nsCOMPtr<nsINodeInfo> nodeInfo;
  nodeInfo = mNodeInfoManager->GetNodeInfo(localName, prefix, nameSpaceID);
  NS_ENSURE_TRUE(nodeInfo, NS_ERROR_OUT_OF_MEMORY);
  
  
  
  nsCOMPtr<nsIAtom> name = nodeInfo->NameAtom();
  if (mSkipLevel != 0 ||
      name == nsGkAtoms::script ||
      name == nsGkAtoms::style) {
    ++mSkipLevel; 
    return NS_OK;
  }  
  
  if (!sAllowedTags || !sAllowedTags->GetEntry(name))
    return NS_OK;
  
  
  nsTArray<const PRUnichar *> allowedAttrs;
  for (PRUint32 i = 0; i < aAttsCount; i += 2) {
    nsContentUtils::SplitExpatName(aAtts[i], getter_AddRefs(prefix),
                                   getter_AddRefs(localName), &nameSpaceID);
    nodeInfo = mNodeInfoManager->GetNodeInfo(localName, prefix, nameSpaceID);
    NS_ENSURE_TRUE(nodeInfo, NS_ERROR_OUT_OF_MEMORY);
    
    name = nodeInfo->NameAtom();
    
    if (nameSpaceID == kNameSpaceID_XMLNS ||
        nameSpaceID == kNameSpaceID_XML ||
        (sAllowedAttributes && sAllowedAttributes->GetEntry(name))) {
      allowedAttrs.AppendElement(aAtts[i]);
      allowedAttrs.AppendElement(aAtts[i + 1]);
    }
  }
  allowedAttrs.AppendElement((const PRUnichar*) nsnull);
  return
    nsXMLFragmentContentSink::HandleStartElement(aName,
                                                 allowedAttrs.Elements(),
                                                 allowedAttrs.Length() - 1,
                                                 aIndex,
                                                 aLineNumber);
}

NS_IMETHODIMP 
nsXHTMLParanoidFragmentSink::HandleEndElement(const PRUnichar *aName)
{
  PRInt32 nameSpaceID;
  nsCOMPtr<nsIAtom> prefix, localName;
  nsContentUtils::SplitExpatName(aName, getter_AddRefs(prefix),
                                 getter_AddRefs(localName), &nameSpaceID);
  
  
  if (nameSpaceID != kNameSpaceID_XHTML) {
    return NS_OK;
  }
  
  nsCOMPtr<nsINodeInfo> nodeInfo;
  nodeInfo = mNodeInfoManager->GetNodeInfo(localName, prefix, nameSpaceID);
  NS_ENSURE_TRUE(nodeInfo, NS_ERROR_OUT_OF_MEMORY);
  
  nsCOMPtr<nsIAtom> name = nodeInfo->NameAtom();
  if (mSkipLevel != 0) {
    --mSkipLevel;
    return NS_OK;
  }

  if (!sAllowedTags || !sAllowedTags->GetEntry(name)) {
    return NS_OK;
  }

  return nsXMLFragmentContentSink::HandleEndElement(aName);
}

NS_IMETHODIMP
nsXHTMLParanoidFragmentSink::
HandleProcessingInstruction(const PRUnichar *aTarget, 
                            const PRUnichar *aData)
{
  
  return NS_OK;
}

NS_IMETHODIMP
nsXHTMLParanoidFragmentSink::HandleComment(const PRUnichar *aName)
{
  
  return NS_OK;
}


NS_IMETHODIMP 
nsXHTMLParanoidFragmentSink::HandleCDataSection(const PRUnichar *aData, 
                                                PRUint32 aLength)
{
  if (mSkipLevel != 0) {
    return NS_OK;
  }

  return nsXMLFragmentContentSink::HandleCDataSection(aData, aLength);
}

NS_IMETHODIMP 
nsXHTMLParanoidFragmentSink::HandleCharacterData(const PRUnichar *aData, 
                                                 PRUint32 aLength)
{
  if (mSkipLevel != 0) {
    return NS_OK;
  }

  return nsXMLFragmentContentSink::HandleCharacterData(aData, aLength);
}
