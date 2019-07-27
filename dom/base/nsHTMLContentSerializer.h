










#ifndef nsHTMLContentSerializer_h__
#define nsHTMLContentSerializer_h__

#include "mozilla/Attributes.h"
#include "nsXHTMLContentSerializer.h"
#include "nsIEntityConverter.h"
#include "nsString.h"

class nsIContent;
class nsIAtom;

class nsHTMLContentSerializer : public nsXHTMLContentSerializer {
 public:
  nsHTMLContentSerializer();
  virtual ~nsHTMLContentSerializer();

  NS_IMETHOD AppendElementStart(mozilla::dom::Element* aElement,
                                mozilla::dom::Element* aOriginalElement,
                                nsAString& aStr) MOZ_OVERRIDE;

  NS_IMETHOD AppendElementEnd(mozilla::dom::Element* aElement,
                              nsAString& aStr) MOZ_OVERRIDE;

  NS_IMETHOD AppendDocumentStart(nsIDocument *aDocument,
                                 nsAString& aStr) MOZ_OVERRIDE;
 protected:

  NS_WARN_UNUSED_RESULT
  virtual bool SerializeHTMLAttributes(nsIContent* aContent,
                                       nsIContent *aOriginalElement,
                                       nsAString& aTagPrefix,
                                       const nsAString& aTagNamespaceURI,
                                       nsIAtom* aTagName,
                                       int32_t aNamespace,
                                       nsAString& aStr);

  NS_WARN_UNUSED_RESULT
  virtual bool AppendAndTranslateEntities(const nsAString& aStr,
                                          nsAString& aOutputStr) MOZ_OVERRIDE;

};

nsresult
NS_NewHTMLContentSerializer(nsIContentSerializer** aSerializer);

#endif
