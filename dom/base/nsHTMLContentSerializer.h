










#ifndef nsHTMLContentSerializer_h__
#define nsHTMLContentSerializer_h__

#include "mozilla/Attributes.h"
#include "nsXHTMLContentSerializer.h"
#include "nsIEntityConverter.h"
#include "nsString.h"

class nsIContent;
class nsIAtom;

class nsHTMLContentSerializer final : public nsXHTMLContentSerializer {
 public:
  nsHTMLContentSerializer();
  virtual ~nsHTMLContentSerializer();

  NS_IMETHOD AppendElementStart(mozilla::dom::Element* aElement,
                                mozilla::dom::Element* aOriginalElement,
                                nsAString& aStr) override;

  NS_IMETHOD AppendElementEnd(mozilla::dom::Element* aElement,
                              nsAString& aStr) override;

  NS_IMETHOD AppendDocumentStart(nsIDocument *aDocument,
                                 nsAString& aStr) override;
 protected:

  MOZ_WARN_UNUSED_RESULT
  virtual bool SerializeHTMLAttributes(nsIContent* aContent,
                                       nsIContent *aOriginalElement,
                                       nsAString& aTagPrefix,
                                       const nsAString& aTagNamespaceURI,
                                       nsIAtom* aTagName,
                                       int32_t aNamespace,
                                       nsAString& aStr);

  MOZ_WARN_UNUSED_RESULT
  virtual bool AppendAndTranslateEntities(const nsAString& aStr,
                                          nsAString& aOutputStr) override;

};

nsresult
NS_NewHTMLContentSerializer(nsIContentSerializer** aSerializer);

#endif
