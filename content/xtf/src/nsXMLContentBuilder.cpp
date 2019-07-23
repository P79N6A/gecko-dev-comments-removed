





































#include "nsCOMPtr.h"
#include "nsString.h"
#include "nsIXMLContentBuilder.h"
#include "nsISupportsArray.h"
#include "nsINameSpaceManager.h"
#include "nsINodeInfo.h"
#include "nsIContent.h"
#include "nsIComponentManager.h"
#include "nsIServiceManager.h"
#include "nsIAtom.h"
#include "nsContentCID.h"
#include "nsIDocument.h"
#include "nsIDOMElement.h"
#include "nsIDOMDocument.h"
#include "nsIDOMText.h"
#include "nsNodeInfoManager.h"
#include "nsContentCreatorFunctions.h"
#include "nsContentUtils.h"

static NS_DEFINE_CID(kXMLDocumentCID, NS_XMLDOCUMENT_CID);

class nsXMLContentBuilder : public nsIXMLContentBuilder
{
protected:
  friend nsresult NS_NewXMLContentBuilder(nsIXMLContentBuilder** aResult);
  
  nsXMLContentBuilder();
  ~nsXMLContentBuilder();
  
public:
  
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSIXMLCONTENTBUILDER

private:
  void EnsureDoc();
  
  nsCOMPtr<nsIContent> mTop;
  nsCOMPtr<nsIContent> mCurrent;
  nsCOMPtr<nsIDocument> mDocument;
  PRInt32 mNamespaceId;
};




nsXMLContentBuilder::nsXMLContentBuilder()
    : mNamespaceId(kNameSpaceID_None)
{
#ifdef DEBUG

#endif
}

nsXMLContentBuilder::~nsXMLContentBuilder()
{
#ifdef DEBUG

#endif
}

nsresult
NS_NewXMLContentBuilder(nsIXMLContentBuilder** aResult)
{
  nsXMLContentBuilder* result = new nsXMLContentBuilder();
  if (! result)
    return NS_ERROR_OUT_OF_MEMORY;

  NS_ADDREF(result);
  *aResult = result;
  return NS_OK;
}




NS_IMPL_ISUPPORTS1(nsXMLContentBuilder, nsIXMLContentBuilder)





NS_IMETHODIMP nsXMLContentBuilder::Clear(nsIDOMElement *root)
{
  mCurrent = do_QueryInterface(root);
  mTop = do_QueryInterface(root);
  if (mNamespaceId != kNameSpaceID_None) {
    mNamespaceId = kNameSpaceID_None;
  }
  return NS_OK;
}


NS_IMETHODIMP nsXMLContentBuilder::SetDocument(nsIDOMDocument *doc)
{
  mDocument = do_QueryInterface(doc);
#ifdef DEBUG
  if (!mDocument)
    NS_WARNING("null document in nsXMLContentBuilder::SetDocument");
#endif
  return NS_OK;
}


NS_IMETHODIMP nsXMLContentBuilder::SetElementNamespace(const nsAString & ns)
{
  nsContentUtils::NameSpaceManager()->RegisterNameSpace(ns, mNamespaceId);
  return NS_OK;
}


NS_IMETHODIMP nsXMLContentBuilder::BeginElement(const nsAString & tagname)
{
  nsCOMPtr<nsIContent> node;
  {
    EnsureDoc();
    nsCOMPtr<nsIAtom> nameAtom = do_GetAtom(tagname);
    mDocument->CreateElem(nameAtom, nsnull, mNamespaceId, PR_FALSE, getter_AddRefs(node));
  }
  if (!node) {
    NS_ERROR("could not create node");
    return NS_ERROR_FAILURE;
  }

  
  
  if (!mCurrent) {
    if (mTop) {
      NS_ERROR("Building of multi-rooted trees not supported");
      return NS_ERROR_FAILURE;
    }
    mTop = node;
    mCurrent = mTop;
  }
  else {    
    mCurrent->AppendChildTo(node, PR_TRUE);
    mCurrent = node;
  }
  
  return NS_OK;
}


NS_IMETHODIMP nsXMLContentBuilder::EndElement()
{
  NS_ASSERTION(mCurrent, "unbalanced begin/endelement");
  mCurrent = mCurrent->GetParent();
  return NS_OK;
}


NS_IMETHODIMP nsXMLContentBuilder::Attrib(const nsAString & name, const nsAString & value)
{
  NS_ASSERTION(mCurrent, "can't set attrib w/o open element");
  nsCOMPtr<nsIAtom> nameAtom = do_GetAtom(name);
  mCurrent->SetAttr(0, nameAtom, value, PR_TRUE);
  return NS_OK;
}


NS_IMETHODIMP nsXMLContentBuilder::TextNode(const nsAString & text)
{
  EnsureDoc();
  NS_ASSERTION(mCurrent, "can't append textnode w/o open element");
  nsCOMPtr<nsIDOMDocument> domDoc = do_QueryInterface(mDocument);
  NS_ASSERTION(domDoc, "no dom document");

  nsCOMPtr<nsIDOMText> textNode;
  domDoc->CreateTextNode(text, getter_AddRefs(textNode));
  NS_ASSERTION(textNode, "Failed to create text node");
  nsCOMPtr<nsIContent> textContent = do_QueryInterface(textNode);
  mCurrent->AppendChildTo(textContent, PR_TRUE);
  return NS_OK;
}


NS_IMETHODIMP nsXMLContentBuilder::GetRoot(nsIDOMElement * *aRoot)
{
  if (!mTop) {
    *aRoot = nsnull;
    return NS_OK;
  }
  return CallQueryInterface(mTop, aRoot);
}


NS_IMETHODIMP nsXMLContentBuilder::GetCurrent(nsIDOMElement * *aCurrent)
{
  if (!mCurrent) {
    *aCurrent = nsnull;
    return NS_OK;
  }  
  return CallQueryInterface(mCurrent, aCurrent);
}




void nsXMLContentBuilder::EnsureDoc()
{
  if (!mDocument) {
    mDocument = do_CreateInstance(kXMLDocumentCID);
    
    
  }
  NS_ASSERTION(mDocument, "null doc");
}
