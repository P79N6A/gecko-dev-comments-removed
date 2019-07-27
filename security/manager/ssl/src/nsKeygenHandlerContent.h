






#ifndef nsKeygenHandlerContent_h
#define nsKeygenHandlerContent_h

#include "mozilla/Attributes.h"
#include "nsIFormProcessor.h"
#include "nsStringFwd.h"
#include "nsTArray.h"

class nsIDOMHTMLElement;

class nsKeygenFormProcessorContent final : public nsIFormProcessor {
public:
  nsKeygenFormProcessorContent();

  virtual nsresult ProcessValue(nsIDOMHTMLElement* aElement,
                                const nsAString& aName,
                                nsAString& aValue) override;

  virtual nsresult ProcessValueIPC(const nsAString& aOldValue,
                                   const nsAString& aChallenge,
                                   const nsAString& aKeyType,
                                   const nsAString& aKeyParams,
                                   nsAString& aNewValue) override;

  virtual nsresult ProvideContent(const nsAString& aFormType,
                                  nsTArray<nsString>& aContent,
                                  nsAString& aAttribute) override;

  NS_DECL_ISUPPORTS

protected:
  ~nsKeygenFormProcessorContent();
};

#endif 
