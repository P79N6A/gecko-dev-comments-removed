










































#ifndef nsXMLContentSerializer_h__
#define nsXMLContentSerializer_h__

#include "nsIContentSerializer.h"
#include "nsISupportsUtils.h"
#include "nsCOMPtr.h"
#include "nsVoidArray.h"

class nsIDOMNode;
class nsIAtom;

class nsXMLContentSerializer : public nsIContentSerializer {
 public:
  nsXMLContentSerializer();
  virtual ~nsXMLContentSerializer();

  NS_DECL_ISUPPORTS

  NS_IMETHOD Init(PRUint32 flags, PRUint32 aWrapColumn,
                  const char* aCharSet, PRBool aIsCopying,
                  PRBool aIsWholeDocument);

  NS_IMETHOD AppendText(nsIDOMText* aText, PRInt32 aStartOffset,
                        PRInt32 aEndOffset, nsAString& aStr);

  NS_IMETHOD AppendCDATASection(nsIDOMCDATASection* aCDATASection,
                                PRInt32 aStartOffset, PRInt32 aEndOffset,
                                nsAString& aStr);

  NS_IMETHOD AppendProcessingInstruction(nsIDOMProcessingInstruction* aPI,
                                         PRInt32 aStartOffset,
                                         PRInt32 aEndOffset,
                                         nsAString& aStr);

  NS_IMETHOD AppendComment(nsIDOMComment* aComment, PRInt32 aStartOffset,
                           PRInt32 aEndOffset, nsAString& aStr);
  
  NS_IMETHOD AppendDoctype(nsIDOMDocumentType *aDoctype,
                           nsAString& aStr);

  NS_IMETHOD AppendElementStart(nsIDOMElement *aElement,
                                nsIDOMElement *aOriginalElement,
                                nsAString& aStr);
  
  NS_IMETHOD AppendElementEnd(nsIDOMElement *aElement,
                              nsAString& aStr);

  NS_IMETHOD Flush(nsAString& aStr) { return NS_OK; }

  NS_IMETHOD AppendDocumentStart(nsIDOMDocument *aDocument,
                                 nsAString& aStr);

 protected:
  virtual void AppendToString(const PRUnichar* aStr,
                              PRInt32 aLength,
                              nsAString& aOutputStr);
  virtual void AppendToString(const PRUnichar aChar, nsAString& aOutputStr);
  virtual void AppendToString(const nsAString& aStr,
                              nsAString& aOutputStr,
                              PRBool aTranslateEntities = PR_FALSE,
                              PRBool aIncrColumn = PR_TRUE);
  nsresult AppendTextData(nsIDOMNode* aNode, 
                          PRInt32 aStartOffset,
                          PRInt32 aEndOffset,
                          nsAString& aStr,
                          PRBool aTranslateEntities,
                          PRBool aIncrColumn);
  virtual nsresult PushNameSpaceDecl(const nsAString& aPrefix,
                                     const nsAString& aURI,
                                     nsIDOMElement* aOwner);
  void PopNameSpaceDeclsFor(nsIDOMElement* aOwner);

  

















  PRBool ConfirmPrefix(nsAString& aPrefix,
                       const nsAString& aURI,
                       nsIDOMElement* aElement,
                       PRBool aIsAttribute);
  


  void GenerateNewPrefix(nsAString& aPrefix);
  void SerializeAttr(const nsAString& aPrefix,
                     const nsAString& aName,
                     const nsAString& aValue,
                     nsAString& aStr,
                     PRBool aDoEscapeEntities);
  PRBool IsShorthandAttr(const nsIAtom* aAttrName,
                         const nsIAtom* aElementName);

  
  
  void MaybeAddNewline(nsAString& aStr);
  void MaybeFlagNewline(nsIDOMNode* aNode);

  PRInt32 mPrefixIndex;
  nsVoidArray mNameSpaceStack;

  
  nsCString mCharset;
  
  PRPackedBool mInAttribute;
  PRPackedBool mAddNewline;
};

nsresult
NS_NewXMLContentSerializer(nsIContentSerializer** aSerializer);

#endif 
