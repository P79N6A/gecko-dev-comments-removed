











































#ifndef nsHTMLContentSerializer_h__
#define nsHTMLContentSerializer_h__

#include "nsXHTMLContentSerializer.h"
#include "nsIEntityConverter.h"
#include "nsString.h"

class nsIContent;
class nsIAtom;

class nsHTMLContentSerializer : public nsXHTMLContentSerializer {
 public:
  nsHTMLContentSerializer();
  virtual ~nsHTMLContentSerializer();

  NS_IMETHOD AppendElementStart(nsIContent *aElement,
                                nsIContent *aOriginalElement,
                                nsAString& aStr);
  
  NS_IMETHOD AppendElementEnd(nsIContent *aElement,
                              nsAString& aStr);

  NS_IMETHOD AppendDocumentStart(nsIDocument *aDocument,
                                 nsAString& aStr);
 protected:

  virtual void SerializeHTMLAttributes(nsIContent* aContent,
                                       nsIContent *aOriginalElement,
                                       nsAString& aTagPrefix,
                                       const nsAString& aTagNamespaceURI,
                                       nsIAtom* aTagName,
                                       nsAString& aStr);

  virtual void AppendAndTranslateEntities(const nsAString& aStr,
                                          nsAString& aOutputStr);

};

nsresult
NS_NewHTMLContentSerializer(nsIContentSerializer** aSerializer);

#endif
