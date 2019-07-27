









#ifndef nsICSSDeclaration_h__
#define nsICSSDeclaration_h__









#include "mozilla/Attributes.h"
#include "nsIDOMCSSStyleDeclaration.h"
#include "nsCSSProperty.h"
#include "CSSValue.h"
#include "nsWrapperCache.h"
#include "nsString.h"
#include "nsIDOMCSSRule.h"
#include "nsIDOMCSSValue.h"
#include "mozilla/ErrorResult.h"
#include "nsAutoPtr.h"
#include "nsCOMPtr.h"

class nsINode;


#define NS_ICSSDECLARATION_IID \
{ 0xdbeabbfa, 0x6cb3, 0x4f5c, \
 { 0xae, 0xc2, 0xdd, 0x55, 0x8d, 0x9d, 0x68, 0x1f } }

class nsICSSDeclaration : public nsIDOMCSSStyleDeclaration,
                          public nsWrapperCache
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ICSSDECLARATION_IID)

  



  NS_IMETHOD GetPropertyValue(const nsCSSProperty aPropID,
                              nsAString& aValue) = 0;

  NS_IMETHOD GetAuthoredPropertyValue(const nsAString& aPropName,
                                      nsAString& aValue) = 0;

  




  NS_IMETHOD SetPropertyValue(const nsCSSProperty aPropID,
                              const nsAString& aValue) = 0;

  virtual nsINode *GetParentObject() = 0;

  
  
  NS_IMETHOD GetCssText(nsAString& aCssText) override = 0;
  NS_IMETHOD SetCssText(const nsAString& aCssText) override = 0;
  NS_IMETHOD GetPropertyValue(const nsAString& aPropName,
                              nsAString& aValue) override = 0;
  virtual already_AddRefed<mozilla::dom::CSSValue>
    GetPropertyCSSValue(const nsAString& aPropertyName,
                        mozilla::ErrorResult& aRv) = 0;
  NS_IMETHOD GetPropertyCSSValue(const nsAString& aProp, nsIDOMCSSValue** aVal) override
  {
    mozilla::ErrorResult error;
    nsRefPtr<mozilla::dom::CSSValue> val = GetPropertyCSSValue(aProp, error);
    if (error.Failed()) {
      return error.StealNSResult();
    }

    nsCOMPtr<nsIDOMCSSValue> xpVal = do_QueryInterface(val);
    xpVal.forget(aVal);
    return NS_OK;
  }
  NS_IMETHOD RemoveProperty(const nsAString& aPropertyName,
                            nsAString& aReturn) override = 0;
  NS_IMETHOD GetPropertyPriority(const nsAString& aPropertyName,
                                 nsAString& aReturn) override = 0;
  NS_IMETHOD SetProperty(const nsAString& aPropertyName,
                         const nsAString& aValue,
                         const nsAString& aPriority) override = 0;
  NS_IMETHOD GetLength(uint32_t* aLength) override = 0;
  NS_IMETHOD Item(uint32_t aIndex, nsAString& aReturn) override
  {
    bool found;
    IndexedGetter(aIndex, found, aReturn);
    if (!found) {
      aReturn.Truncate();
    }
    return NS_OK;
  }
  NS_IMETHOD GetParentRule(nsIDOMCSSRule * *aParentRule) override = 0;

  
  void SetCssText(const nsAString& aString, mozilla::ErrorResult& rv) {
    rv = SetCssText(aString);
  }
  void GetCssText(nsString& aString) {
    
    
    GetCssText(static_cast<nsAString&>(aString));
  }
  uint32_t Length() {
    uint32_t length;
    GetLength(&length);
    return length;
  }
  void Item(uint32_t aIndex, nsString& aPropName) {
    Item(aIndex, static_cast<nsAString&>(aPropName));
  }

  
  virtual void IndexedGetter(uint32_t aIndex, bool& aFound, nsAString& aPropName) = 0;

  void GetPropertyValue(const nsAString& aPropName, nsString& aValue,
                        mozilla::ErrorResult& rv) {
    rv = GetPropertyValue(aPropName, aValue);
  }
  void GetAuthoredPropertyValue(const nsAString& aPropName, nsString& aValue,
                                mozilla::ErrorResult& rv) {
    rv = GetAuthoredPropertyValue(aPropName, aValue);
  }
  void GetPropertyPriority(const nsAString& aPropName, nsString& aPriority) {
    GetPropertyPriority(aPropName, static_cast<nsAString&>(aPriority));
  }
  void SetProperty(const nsAString& aPropName, const nsAString& aValue,
                   const nsAString& aPriority, mozilla::ErrorResult& rv) {
    rv = SetProperty(aPropName, aValue, aPriority);
  }
  void RemoveProperty(const nsAString& aPropName, nsString& aRetval,
                      mozilla::ErrorResult& rv) {
    rv = RemoveProperty(aPropName, aRetval);
  }
  already_AddRefed<nsIDOMCSSRule> GetParentRule() {
    nsCOMPtr<nsIDOMCSSRule> rule;
    GetParentRule(getter_AddRefs(rule));
    return rule.forget();
  }
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsICSSDeclaration, NS_ICSSDECLARATION_IID)

#define NS_DECL_NSICSSDECLARATION                                   \
  NS_IMETHOD GetPropertyValue(const nsCSSProperty aPropID,          \
                              nsAString& aValue) override;          \
  NS_IMETHOD GetAuthoredPropertyValue(const nsAString& aPropName,   \
                                      nsAString& aValue) override;  \
  NS_IMETHOD SetPropertyValue(const nsCSSProperty aPropID,          \
                              const nsAString& aValue) override;

#define NS_DECL_NSIDOMCSSSTYLEDECLARATION_HELPER \
  NS_IMETHOD GetCssText(nsAString & aCssText) override; \
  NS_IMETHOD SetCssText(const nsAString & aCssText) override; \
  NS_IMETHOD GetPropertyValue(const nsAString & propertyName, nsAString & _retval) override; \
  NS_IMETHOD RemoveProperty(const nsAString & propertyName, nsAString & _retval) override; \
  NS_IMETHOD GetPropertyPriority(const nsAString & propertyName, nsAString & _retval) override; \
  NS_IMETHOD SetProperty(const nsAString & propertyName, const nsAString & value, const nsAString & priority) override; \
  NS_IMETHOD GetLength(uint32_t *aLength) override; \
  NS_IMETHOD Item(uint32_t index, nsAString & _retval) override; \
  NS_IMETHOD GetParentRule(nsIDOMCSSRule * *aParentRule) override;

#endif 
