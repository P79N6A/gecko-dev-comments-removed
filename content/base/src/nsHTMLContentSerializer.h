










































#ifndef nsHTMLContentSerializer_h__
#define nsHTMLContentSerializer_h__

#include "nsXMLContentSerializer.h"
#include "nsIEntityConverter.h"
#include "nsString.h"
#include "nsILineBreaker.h"

class nsIContent;
class nsIAtom;

class nsHTMLContentSerializer : public nsXMLContentSerializer {
 public:
  nsHTMLContentSerializer();
  virtual ~nsHTMLContentSerializer();

  NS_IMETHOD Init(PRUint32 flags, PRUint32 aWrapColumn,
                  const char* aCharSet, PRBool aIsCopying,
                  PRBool aIsWholeDocument);

  NS_IMETHOD AppendText(nsIDOMText* aText, 
                        PRInt32 aStartOffset,
                        PRInt32 aEndOffset,
                        nsAString& aStr);
  NS_IMETHOD AppendElementStart(nsIDOMElement *aElement,
                                nsIDOMElement *aOriginalElement,
                                nsAString& aStr);
  
  NS_IMETHOD AppendElementEnd(nsIDOMElement *aElement,
                              nsAString& aStr);

  NS_IMETHOD AppendDocumentStart(nsIDOMDocument *aDocument,
                                 nsAString& aStr);
 protected:
  PRBool LineBreakBeforeOpen(nsIAtom* aName, PRBool aHasDirtyAttr);
  PRBool LineBreakAfterOpen(nsIAtom* aName, PRBool aHasDirtyAttr);
  PRBool LineBreakBeforeClose(nsIAtom* aName, PRBool aHasDirtyAttr);
  PRBool LineBreakAfterClose(nsIAtom* aName, PRBool aHasDirtyAttr);
  PRBool IsFirstChildOfOL(nsIDOMElement* aElement);
  void StartIndentation(nsIAtom* aName, 
                        PRBool aHasDirtyAttr,
                        nsAString& aStr);
  void EndIndentation(nsIAtom* aName, 
                      PRBool aHasDirtyAttr,
                      nsAString& aStr);
  nsresult GetEntityConverter(nsIEntityConverter** aConverter);
  void SerializeAttributes(nsIContent* aContent,
                           nsIAtom* aTagName,
                           nsAString& aStr);
  void SerializeLIValueAttribute(nsIDOMElement* aElement,
                                 nsAString& aStr);
  virtual void AppendToString(const PRUnichar* aStr,
                              PRInt32 aLength,
                              nsAString& aOutputStr);
  virtual void AppendToString(const PRUnichar aChar,
                              nsAString& aOutputStr);
  virtual void AppendToString(const nsAString& aStr,
                              nsAString& aOutputStr,
                              PRBool aTranslateEntities = PR_FALSE,
                              PRBool aIncrColumn = PR_TRUE);
  virtual void AppendToStringConvertLF(const nsAString& aStr,
                                       nsAString& aOutputStr);
  void AppendWrapped_WhitespaceSequence(
          nsASingleFragmentString::const_char_iterator &aPos,
          const nsASingleFragmentString::const_char_iterator aEnd,
          const nsASingleFragmentString::const_char_iterator aSequenceStart,
          PRBool &aMayIgnoreStartOfLineWhitespaceSequence,
          nsAString &aOutputStr);
  void AppendWrapped_NonWhitespaceSequence(
          nsASingleFragmentString::const_char_iterator &aPos,
          const nsASingleFragmentString::const_char_iterator aEnd,
          const nsASingleFragmentString::const_char_iterator aSequenceStart,
          PRBool &aMayIgnoreStartOfLineWhitespaceSequence,
          nsAString &aOutputStr);
  virtual void AppendToStringWrapped(const nsASingleFragmentString& aStr,
                                     nsAString& aOutputStr,
                                     PRBool aTranslateEntities);
  PRBool HasLongLines(const nsString& text, PRInt32& aLastNewlineOffset);
  nsresult EscapeURI(const nsAString& aURI, nsAString& aEscapedURI);
  PRBool IsJavaScript(nsIAtom* aAttrNameAtom, const nsAString& aAttrValueString);

  nsCOMPtr<nsIEntityConverter> mEntityConverter;

  PRInt32   mIndent;
  PRInt32   mColPos;
  PRUint32  mFlags;
  PRPackedBool  mInBody;

  PRPackedBool  mDoFormat;
  PRPackedBool  mDoHeader;
  PRPackedBool  mBodyOnly;
  PRPackedBool  mIsCopying; 

  
  
  
  PRPackedBool  mAddSpace;
  PRPackedBool  mMayIgnoreLineBreakSequence;

  
  
  PRPackedBool  mIsWholeDocument;

  
  PRPackedBool  mIsFirstChildOfOL;
  PRInt32       mPreLevel;

  








  PRPackedBool mInCDATA;
  PRPackedBool mNeedLineBreaker;

  nsCOMPtr<nsILineBreaker> mLineBreaker;

  PRInt32   mMaxColumn;
  nsString  mLineBreak;

  
  struct olState {
    olState(PRInt32 aStart, PRBool aIsFirst):startVal(aStart),isFirstListItem(aIsFirst)
    {
    }
    PRInt32 startVal;
    PRBool isFirstListItem;
  };

  nsAutoVoidArray   mOLStateStack;
};

nsresult
NS_NewHTMLContentSerializer(nsIContentSerializer** aSerializer);

#endif
