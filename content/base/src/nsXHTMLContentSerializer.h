











































#ifndef nsXHTMLContentSerializer_h__
#define nsXHTMLContentSerializer_h__

#include "nsXMLContentSerializer.h"
#include "nsIEntityConverter.h"
#include "nsString.h"
#include "nsTArray.h"

class nsIContent;
class nsIAtom;

class nsXHTMLContentSerializer : public nsXMLContentSerializer {
 public:
  nsXHTMLContentSerializer();
  virtual ~nsXHTMLContentSerializer();

  NS_IMETHOD Init(PRUint32 flags, PRUint32 aWrapColumn,
                  const char* aCharSet, PRBool aIsCopying,
                  PRBool aRewriteEncodingDeclaration);

  NS_IMETHOD AppendText(nsIDOMText* aText,
                        PRInt32 aStartOffset,
                        PRInt32 aEndOffset,
                        nsAString& aStr);

  NS_IMETHOD AppendDocumentStart(nsIDOMDocument *aDocument,
                                 nsAString& aStr);

 protected:


  virtual PRBool CheckElementStart(nsIContent * aContent,
                          PRBool & aForceFormat,
                          nsAString& aStr);

  virtual void AppendEndOfElementStart(nsIDOMElement *aOriginalElement,
                               nsIAtom * aName,
                               PRInt32 aNamespaceID,
                               nsAString& aStr);

  virtual void AfterElementStart(nsIContent * aContent,
                         nsIDOMElement *aOriginalElement,
                         nsAString& aStr);

  virtual PRBool CheckElementEnd(nsIContent * aContent,
                          PRBool & aForceFormat,
                          nsAString& aStr);

  virtual void AfterElementEnd(nsIContent * aContent,
                               nsAString& aStr);

  virtual PRBool LineBreakBeforeOpen(PRInt32 aNamespaceID, nsIAtom* aName);
  virtual PRBool LineBreakAfterOpen(PRInt32 aNamespaceID, nsIAtom* aName);
  virtual PRBool LineBreakBeforeClose(PRInt32 aNamespaceID, nsIAtom* aName);
  virtual PRBool LineBreakAfterClose(PRInt32 aNamespaceID, nsIAtom* aName);

  PRBool HasLongLines(const nsString& text, PRInt32& aLastNewlineOffset);

  
  virtual void MaybeEnterInPreContent(nsIContent* aNode);
  virtual void MaybeLeaveFromPreContent(nsIContent* aNode);

  virtual void SerializeAttributes(nsIContent* aContent,
                           nsIDOMElement *aOriginalElement,
                           nsAString& aTagPrefix,
                           const nsAString& aTagNamespaceURI,
                           nsIAtom* aTagName,
                           nsAString& aStr,
                           PRUint32 aSkipAttr,
                           PRBool aAddNSAttr);

  PRBool IsFirstChildOfOL(nsIDOMElement* aElement);

  void SerializeLIValueAttribute(nsIDOMElement* aElement,
                                 nsAString& aStr);
  PRBool IsShorthandAttr(const nsIAtom* aAttrName,
                         const nsIAtom* aElementName);

  virtual void AppendToString(const PRUnichar* aStr,
                              PRInt32 aLength,
                              nsAString& aOutputStr);
  virtual void AppendToString(const PRUnichar aChar,
                              nsAString& aOutputStr);
  virtual void AppendToString(const nsAString& aStr,
                              nsAString& aOutputStr);
  virtual void AppendAndTranslateEntities(const nsAString& aStr,
                                          nsAString& aOutputStr);

  virtual void AppendToStringConvertLF(const nsAString& aStr,
                                       nsAString& aOutputStr);

  virtual void AppendToStringWrapped(const nsASingleFragmentString& aStr,
                                     nsAString& aOutputStr);

  virtual void AppendToStringFormatedWrapped(const nsASingleFragmentString& aStr,
                                             nsAString& aOutputStr);

  nsresult EscapeURI(nsIContent* aContent,
                     const nsAString& aURI,
                     nsAString& aEscapedURI);

  nsCOMPtr<nsIEntityConverter> mEntityConverter;

  PRInt32  mInBody;

  



  PRPackedBool  mIsHTMLSerializer;

  PRPackedBool  mDoHeader;
  PRPackedBool  mBodyOnly;
  PRPackedBool  mIsCopying; 

  








  PRInt32 mDisableEntityEncoding;

  
  
  PRPackedBool  mRewriteEncodingDeclaration;

  
  PRPackedBool  mIsFirstChildOfOL;

  
  struct olState {
    olState(PRInt32 aStart, PRBool aIsFirst)
      : startVal(aStart),
        isFirstListItem(aIsFirst)
    {
    }

    olState(const olState & aOlState)
    {
      startVal = aOlState.startVal;
      isFirstListItem = aOlState.isFirstListItem;
    }

    
    PRInt32 startVal;

    
    
    PRBool isFirstListItem;
  };

  
  nsAutoTArray<olState, 8> mOLStateStack;

};

nsresult
NS_NewXHTMLContentSerializer(nsIContentSerializer** aSerializer);

#endif
