



































#include <stdio.h>
#include "nscore.h"
#include "nsIAtom.h"
#include "nsCRT.h"
#include "nsHTMLParts.h"
#include "nsGenericHTMLElement.h"
#include "nsString.h"
#include "nsIDocument.h"
#include "nsISupportsArray.h"
#include "nsDocument.h"
#include "nsIURL.h"
#include "nsIDOMText.h"
#include "nsINameSpaceManager.h"

void testAttributes(nsGenericHTMLElement* content) {
  nsIAtom* sBORDER = NS_NewAtom("border");
  nsIAtom* sHEIGHT = NS_NewAtom("height");
  nsIAtom* sSRC = NS_NewAtom("src");
  nsIAtom* sBAD = NS_NewAtom("badattribute");
  nsString sempty;
  nsString sfoo_gif(NS_LITERAL_STRING("foo.gif"));

  content->SetAttr(kNameSpaceID_None, sBORDER, EmptyString(), PR_FALSE);
  content->SetAttribute(kNameSpaceID_None, sHEIGHT, sempty, PR_FALSE);
  content->SetAttribute(kNameSpaceID_None, sSRC, sfoo_gif, PR_FALSE);

  const nsAttrValue* attr;
  attr = content->GetParsedAttr(sBORDER);
  if (!attr || attr->Type() != nsAttrValue::eString) {
    printf("test 0 failed\n");
  }

  attr = content->GetParsedAttr(sBAD);
  if (attr) {
    printf("test 2 failed\n");
  }

  content->UnsetAttribute(kNameSpaceID_None, sWIDTH, PR_FALSE);

  nsISupportsArray* allNames;
  NS_NewISupportsArray(&allNames);
  if (nsnull == allNames) return;

  PRInt32 na;
  content->GetAttributeCount(na);
  if (na != 3) {
    printf("test 5 (unset attriubte) failed\n");
  }
  PRInt32 index;
  for (index = 0; index < na; index++) {
    nsIAtom* name, *prefix = nsnull;
    PRInt32 nameSpaceID;
    content->GetAttributeNameAt(index, nameSpaceID, name, prefix);
    allNames->AppendElement(name);
    NS_RELEASE(name);
    NS_IF_RELEASE(prefix);
  }

  PRBool borderFound = PR_FALSE,heightFound = PR_FALSE,srcFound = PR_FALSE;
  for (int n = 0; n < 3; n++) {
    const nsIAtom* ident = (const nsIAtom*)allNames->ElementAt(n);
    if (sBORDER == ident) {
      borderFound = PR_TRUE;
    }
    if (sHEIGHT == ident) {
      heightFound = PR_TRUE;
    }
    if (sSRC == ident) {
      srcFound = PR_TRUE;
    }
  }
  if (!(borderFound && heightFound && srcFound)) {
    printf("test 6 failed\n");
  }

  NS_RELEASE(allNames);

  NS_RELEASE(sBORDER);
  NS_RELEASE(sWIDTH);
  NS_RELEASE(sHEIGHT);
  NS_RELEASE(sSRC);
}

void testStrings(nsIDocument* aDoc) {
  printf("begin string tests\n");

  PRBool val;
  
  val = (NS_ConvertASCIItoUTF16("mrString")).EqualsLiteral("mrString"); 
  if (PR_TRUE != val) {
    printf("test 0 failed\n");
  }
  val = (NS_ConvertASCIItoUTF16("mrString")).EqualsLiteral("MRString"); 
  if (PR_FALSE != val) {
    printf("test 1 failed\n");
  }
  val = (NS_ConvertASCIItoUTF16("mrString")).EqualsLiteral("mrStri"); 
  if (PR_FALSE != val) {
    printf("test 2 failed\n");
  }
  val = (NS_ConvertASCIItoUTF16("mrStri")).EqualsLiteral("mrString"); 
  if (PR_FALSE != val) {
    printf("test 3 failed\n");
  }
  
  val = (NS_ConvertASCIItoUTF16("mrString")).LowerCaseEqualsLiteral("mrstring");
  if (PR_TRUE != val) {
    printf("test 4 failed\n");
  }
  val = (NS_ConvertASCIItoUTF16("mrString")).LowerCaseEqualsLiteral("mrstring");
  if (PR_TRUE != val) {
    printf("test 5 failed\n");
  }
  val = (NS_ConvertASCIItoUTF16("mrString")).LowerCaseEqualsLiteral("mrstri");
  if (PR_FALSE != val) {
    printf("test 6 failed\n");
  }
  val = (NS_ConvertASCIItoUTF16("mrStri")).LowerCaseEqualsLiteral("mrstring");
  if (PR_FALSE != val) {
    printf("test 7 failed\n");
  }
  
  val = (NS_ConvertASCIItoUTF16("mrString")).EqualsIgnoreCase(NS_NewAtom("mrString"));
  if (PR_TRUE != val) {
    printf("test 8 failed\n");
  }
  val = (NS_ConvertASCIItoUTF16("mrString")).EqualsIgnoreCase(NS_NewAtom("MRStrINg"));
  if (PR_TRUE != val) {
    printf("test 9 failed\n");
  }
  val = (NS_ConvertASCIItoUTF16("mrString")).EqualsIgnoreCase(NS_NewAtom("mrStri"));
  if (PR_FALSE != val) {
    printf("test 10 failed\n");
  }
  val = (NS_ConvertASCIItoUTF16("mrStri")).EqualsIgnoreCase(NS_NewAtom("mrString"));
  if (PR_FALSE != val) {
    printf("test 11 failed\n");
  }

  printf("string tests complete\n");
}

class MyDocument : public nsDocument {
public:
  MyDocument();
  NS_IMETHOD StartDocumentLoad(const char* aCommand,
                               nsIChannel* aChannel,
                               nsILoadGroup* aLoadGroup,
                               nsISupports* aContainer,
                               nsIStreamListener **aDocListener)
  {
    return NS_OK;
  }

  NS_IMETHOD    ImportNode(nsIDOMNode* aImportedNode, PRBool aDeep, nsIDOMNode** aReturn) {
    return NS_OK;
  }

  NS_IMETHOD    CreateElementNS(const nsAString& aNamespaceURI, const nsAString& aQualifiedName, nsIDOMElement** aReturn) {
    return NS_OK;
  }

  NS_IMETHOD    CreateAttributeNS(const nsAString& aNamespaceURI, const nsAString& aQualifiedName, nsIDOMAttr** aReturn) {
    return NS_OK;
  }

  NS_IMETHOD    GetElementsByTagNameNS(const nsAString& aNamespaceURI, const nsAString& aLocalName, nsIDOMNodeList** aReturn) {
    return NS_OK;
  }

  NS_IMETHOD    GetElementById(const nsAString& aElementId, nsIDOMElement** aReturn) {
    return NS_OK;
  }

protected:
  virtual ~MyDocument();
};

MyDocument::MyDocument()
{
}

MyDocument::~MyDocument()
{
}

int main(int argc, char** argv)
{
  




  
  static const char* srcStr = "This is some meaningless text about nothing at all";
  nsresult rv;
  PRUint32 origSrcLen = nsCRT::strlen((char *)srcStr);
  const int BUFFER_LENGTH = 100;
  PRUnichar destStr[BUFFER_LENGTH];
  PRUint32 srcLen = origSrcLen;
  PRUint32 destLen = BUFFER_LENGTH;
  
  for (PRUint32 i=0; i<srcLen; i++) destStr[i] = ((PRUint8)srcStr[i]);

  
  MyDocument *myDoc = new MyDocument();
  if (myDoc) {
    testStrings(myDoc);
  } else {
    printf("Out of memory trying to create document\n");
    return -1;
  }


  
  nsIContent *text;
  rv = NS_NewTextNode(&text, myDoc->NodeInfoManager());
  if (NS_OK != rv) {
    printf("Could not create text content.\n");
    return -1;
  }

  nsIDOMText* txt = nsnull;
  text->QueryInterface(NS_GET_IID(nsIDOMText), (void**) &txt);
  nsAutoString tmp(destStr);
  txt->AppendData(tmp);
  NS_RELEASE(txt);

  rv = text->BindToTree(myDoc, nsnull, nsnull, PR_FALSE);
  if (NS_FAILED(rv)) {
    printf("Could not bind text content to tree.\n");
    text->UnbindFromTree();
    return -1;
  }

#if 0
  
  nsIContent* textContent;
  rv = text->QueryInterface(NS_GET_IID(nsIContent),(void **)&textContent);
  if (NS_OK != rv) {
    printf("Created text content does not have the IContent interface.\n");
    return -1;
  }

  
  nsAutoString stringBuf;
  textContent->GetText(stringBuf,0,textContent->GetLength());
  if (!stringBuf.Equals(nsString(destStr,destLen))) {
    printf("something wrong with the text in a text content\n");
  }
#endif

  
  nsGenericHTMLElement* container;
  nsIAtom* li = NS_NewAtom("li");

  nsCOMPtr<nsINodeInfo> ni;
  myDoc->NodeInfoManager()->GetNodeInfo(li, nsnull, kNameSpaceID_None,
                                        getter_AddRefs(ni));

  rv = NS_NewHTMLLIElement(&container,ni);
  if (NS_OK != rv) {
    printf("Could not create container.\n");
    return -1;
  }

  container->AppendChildTo(text, PR_FALSE);
  PRInt32 nk;
  container->ChildCount(nk);
  if (nk != 1) {
    printf("Container has wrong number of children.");
  }

  printf("begin attribute tests\n");
  testAttributes(container);
  printf("attribute tests complete\n");


  
  text->Release(); 
  delete container;
  delete text;
  myDoc->Release();
  return 0;
}
