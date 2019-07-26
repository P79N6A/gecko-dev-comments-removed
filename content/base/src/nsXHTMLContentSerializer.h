










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

  NS_IMETHOD Init(uint32_t flags, uint32_t aWrapColumn,
                  const char* aCharSet, bool aIsCopying,
                  bool aRewriteEncodingDeclaration);

  NS_IMETHOD AppendText(nsIContent* aText,
                        int32_t aStartOffset,
                        int32_t aEndOffset,
                        nsAString& aStr);

  NS_IMETHOD AppendDocumentStart(nsIDocument *aDocument,
                                 nsAString& aStr);

 protected:


  virtual bool CheckElementStart(nsIContent * aContent,
                          bool & aForceFormat,
                          nsAString& aStr);

  virtual void AppendEndOfElementStart(nsIContent *aOriginalElement,
                               nsIAtom * aName,
                               int32_t aNamespaceID,
                               nsAString& aStr);

  virtual void AfterElementStart(nsIContent * aContent,
                         nsIContent *aOriginalElement,
                         nsAString& aStr);

  virtual bool CheckElementEnd(nsIContent * aContent,
                          bool & aForceFormat,
                          nsAString& aStr);

  virtual void AfterElementEnd(nsIContent * aContent,
                               nsAString& aStr);

  virtual bool LineBreakBeforeOpen(int32_t aNamespaceID, nsIAtom* aName);
  virtual bool LineBreakAfterOpen(int32_t aNamespaceID, nsIAtom* aName);
  virtual bool LineBreakBeforeClose(int32_t aNamespaceID, nsIAtom* aName);
  virtual bool LineBreakAfterClose(int32_t aNamespaceID, nsIAtom* aName);

  bool HasLongLines(const nsString& text, int32_t& aLastNewlineOffset);

  
  virtual void MaybeEnterInPreContent(nsIContent* aNode);
  virtual void MaybeLeaveFromPreContent(nsIContent* aNode);

  virtual void SerializeAttributes(nsIContent* aContent,
                           nsIContent *aOriginalElement,
                           nsAString& aTagPrefix,
                           const nsAString& aTagNamespaceURI,
                           nsIAtom* aTagName,
                           nsAString& aStr,
                           uint32_t aSkipAttr,
                           bool aAddNSAttr);

  bool IsFirstChildOfOL(nsIContent* aElement);

  void SerializeLIValueAttribute(nsIContent* aElement,
                                 nsAString& aStr);
  bool IsShorthandAttr(const nsIAtom* aAttrName,
                         const nsIAtom* aElementName);
  virtual void AppendAndTranslateEntities(const nsAString& aStr,
                                          nsAString& aOutputStr);
  nsresult EscapeURI(nsIContent* aContent,
                     const nsAString& aURI,
                     nsAString& aEscapedURI);

  nsCOMPtr<nsIEntityConverter> mEntityConverter;

  



  bool          mIsHTMLSerializer;

  bool          mDoHeader;
  bool          mIsCopying; 

  








  int32_t mDisableEntityEncoding;

  
  
  bool          mRewriteEncodingDeclaration;

  
  bool          mIsFirstChildOfOL;

  
  struct olState {
    olState(int32_t aStart, bool aIsFirst)
      : startVal(aStart),
        isFirstListItem(aIsFirst)
    {
    }

    olState(const olState & aOlState)
    {
      startVal = aOlState.startVal;
      isFirstListItem = aOlState.isFirstListItem;
    }

    
    int32_t startVal;

    
    
    bool isFirstListItem;
  };

  
  nsAutoTArray<olState, 8> mOLStateStack;

  bool HasNoChildren(nsIContent* aContent);
};

nsresult
NS_NewXHTMLContentSerializer(nsIContentSerializer** aSerializer);

#endif
