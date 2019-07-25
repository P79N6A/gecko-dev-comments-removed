











































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

  NS_IMETHOD AppendText(nsIContent* aText,
                        PRInt32 aStartOffset,
                        PRInt32 aEndOffset,
                        nsAString& aStr);

  NS_IMETHOD AppendDocumentStart(nsIDocument *aDocument,
                                 nsAString& aStr);

 protected:


  virtual PRBool CheckElementStart(nsIContent * aContent,
                          PRBool & aForceFormat,
                          nsAString& aStr);

  virtual void AppendEndOfElementStart(nsIContent *aOriginalElement,
                               nsIAtom * aName,
                               PRInt32 aNamespaceID,
                               nsAString& aStr);

  virtual void AfterElementStart(nsIContent * aContent,
                         nsIContent *aOriginalElement,
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
                           nsIContent *aOriginalElement,
                           nsAString& aTagPrefix,
                           const nsAString& aTagNamespaceURI,
                           nsIAtom* aTagName,
                           nsAString& aStr,
                           PRUint32 aSkipAttr,
                           PRBool aAddNSAttr);

  PRBool IsFirstChildOfOL(nsIContent* aElement);

  void SerializeLIValueAttribute(nsIContent* aElement,
                                 nsAString& aStr);
  PRBool IsShorthandAttr(const nsIAtom* aAttrName,
                         const nsIAtom* aElementName);
  virtual void AppendAndTranslateEntities(const nsAString& aStr,
                                          nsAString& aOutputStr);
  nsresult EscapeURI(nsIContent* aContent,
                     const nsAString& aURI,
                     nsAString& aEscapedURI);

  nsCOMPtr<nsIEntityConverter> mEntityConverter;

  



  PRPackedBool  mIsHTMLSerializer;

  PRPackedBool  mDoHeader;
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

  PRBool HasNoChildren(nsIContent* aContent);
};

nsresult
NS_NewXHTMLContentSerializer(nsIContentSerializer** aSerializer);

#endif
