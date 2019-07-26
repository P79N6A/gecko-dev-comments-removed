









#ifndef nsICSSDeclaration_h__
#define nsICSSDeclaration_h__









#include "nsIDOMCSSStyleDeclaration.h"
#include "nsCSSProperty.h"
#include "nsWrapperCache.h"
#include "mozilla/dom/BindingUtils.h"
#include "nsString.h"
#include "nsIDOMCSSRule.h"
#include "nsIDOMCSSValue.h"
#include "mozilla/ErrorResult.h"


#define NS_ICSSDECLARATION_IID \
{ 0xdbeabbfa, 0x6cb3, 0x4f5c, \
 { 0xae, 0xc2, 0xdd, 0x55, 0x8d, 0x9d, 0x68, 0x1f } }

class nsINode;

class nsICSSDeclaration : public nsIDOMCSSStyleDeclaration,
                          public nsWrapperCache
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ICSSDECLARATION_IID)

  



  NS_IMETHOD GetPropertyValue(const nsCSSProperty aPropID,
                              nsAString& aValue) = 0;

  




  NS_IMETHOD SetPropertyValue(const nsCSSProperty aPropID,
                              const nsAString& aValue) = 0;

  virtual nsINode *GetParentObject() = 0;

  
  
  NS_IMETHOD GetCssText(nsAString& aCssText) = 0;
  NS_IMETHOD SetCssText(const nsAString& aCssText) = 0;
  NS_IMETHOD GetPropertyValue(const nsAString& aPropName,
                              nsAString& aValue) = 0;
  NS_IMETHOD GetPropertyCSSValue(const nsAString& aPropertyName,
                                 nsIDOMCSSValue** aReturn) = 0;
  NS_IMETHOD RemoveProperty(const nsAString& aPropertyName,
                            nsAString& aReturn) = 0;
  NS_IMETHOD GetPropertyPriority(const nsAString& aPropertyName,
                                 nsAString& aReturn) = 0;
  NS_IMETHOD SetProperty(const nsAString& aPropertyName,
                         const nsAString& aValue,
                         const nsAString& aPriority) = 0;
  NS_IMETHOD GetLength(uint32_t* aLength) = 0;
  NS_IMETHOD Item(uint32_t aIndex, nsAString& aReturn)
  {
    bool found;
    IndexedGetter(aIndex, found, aReturn);
    if (!found) {
      aReturn.Truncate();
    }
    return NS_OK;
  }
  NS_IMETHOD GetParentRule(nsIDOMCSSRule * *aParentRule) = 0;

  
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
  already_AddRefed<nsIDOMCSSValue>
    GetPropertyCSSValue(const nsAString& aPropName, mozilla::ErrorResult& rv) {
    nsCOMPtr<nsIDOMCSSValue> val;
    rv = GetPropertyCSSValue(aPropName, getter_AddRefs(val));
    return val.forget();
  }
  void GetPropertyPriority(const nsAString& aPropName, nsString& aPriority) {
    GetPropertyPriority(aPropName, static_cast<nsAString&>(aPriority));
  }
  
  void SetProperty(const nsAString& aPropName, const nsAString& aValue,
                   const mozilla::dom::Optional<nsAString>& aPriority,
                   mozilla::ErrorResult& rv) {
    if (aPriority.WasPassed()) {
      rv = SetProperty(aPropName, aValue, aPriority.Value());
    } else {
      rv = SetProperty(aPropName, aValue, EmptyString());
    }
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

#define NS_DECL_NSICSSDECLARATION                               \
  NS_IMETHOD GetPropertyValue(const nsCSSProperty aPropID,    \
                              nsAString& aValue);               \
  NS_IMETHOD SetPropertyValue(const nsCSSProperty aPropID,    \
                              const nsAString& aValue);

#endif 
