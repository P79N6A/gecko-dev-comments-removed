






#ifndef nsDOMCSSDeclaration_h___
#define nsDOMCSSDeclaration_h___

#include "mozilla/Attributes.h"
#include "nsICSSDeclaration.h"
#include "nsCOMPtr.h"
#include "mozilla/dom/CSS2PropertiesBinding.h"

class nsIPrincipal;
class nsIDocument;

namespace mozilla {
namespace css {
class Declaration;
class Loader;
class Rule;
}
}

class nsDOMCSSDeclaration : public nsICSSDeclaration
{
public:
  
  
  NS_IMETHOD QueryInterface(REFNSIID aIID, void** aInstancePtr);

  
  
  
  NS_IMETHOD_(nsrefcnt) AddRef() = 0;
  NS_IMETHOD_(nsrefcnt) Release() = 0;

  NS_DECL_NSICSSDECLARATION
  using nsICSSDeclaration::GetLength;

  
  
  NS_IMETHOD GetCssText(nsAString & aCssText);
  NS_IMETHOD SetCssText(const nsAString & aCssText) MOZ_OVERRIDE;
  NS_IMETHOD GetPropertyValue(const nsAString & propertyName,
                              nsAString & _retval) MOZ_OVERRIDE;
  virtual already_AddRefed<mozilla::dom::CSSValue>
    GetPropertyCSSValue(const nsAString & propertyName,
                        mozilla::ErrorResult& aRv) MOZ_OVERRIDE;
  using nsICSSDeclaration::GetPropertyCSSValue;
  NS_IMETHOD RemoveProperty(const nsAString & propertyName,
                            nsAString & _retval);
  NS_IMETHOD GetPropertyPriority(const nsAString & propertyName,
                                 nsAString & _retval) MOZ_OVERRIDE;
  NS_IMETHOD SetProperty(const nsAString & propertyName,
                         const nsAString & value, const nsAString & priority) MOZ_OVERRIDE;
  NS_IMETHOD GetLength(uint32_t *aLength) MOZ_OVERRIDE;
  NS_IMETHOD GetParentRule(nsIDOMCSSRule * *aParentRule) MOZ_OVERRIDE = 0;

  
#define CSS_PROP_DOMPROP_PREFIXED(prop_) Moz ## prop_
#define CSS_PROP(name_, id_, method_, flags_, pref_, parsevariant_,          \
                 kwtable_, stylestruct_, stylestructoffset_, animtype_)      \
  void                                                                       \
  Get##method_(nsAString& aValue, mozilla::ErrorResult& rv)                  \
  {                                                                          \
    rv = GetPropertyValue(eCSSProperty_##id_, aValue);                       \
  }                                                                          \
                                                                             \
  void                                                                       \
  Set##method_(const nsAString& aValue, mozilla::ErrorResult& rv)            \
  {                                                                          \
    rv = SetPropertyValue(eCSSProperty_##id_, aValue);                       \
  }

#define CSS_PROP_LIST_EXCLUDE_INTERNAL
#define CSS_PROP_SHORTHAND(name_, id_, method_, flags_, pref_)  \
  CSS_PROP(name_, id_, method_, flags_, pref_, X, X, X, X, X)
#include "nsCSSPropList.h"

#define CSS_PROP_ALIAS(aliasname_, propid_, aliasmethod_, pref_)  \
  CSS_PROP(X, propid_, aliasmethod_, X, pref_, X, X, X, X, X)
#include "nsCSSPropAliasList.h"
#undef CSS_PROP_ALIAS

#undef CSS_PROP_SHORTHAND
#undef CSS_PROP_LIST_EXCLUDE_INTERNAL
#undef CSS_PROP
#undef CSS_PROP_DOMPROP_PREFIXED

  virtual void IndexedGetter(uint32_t aIndex, bool& aFound, nsAString& aPropName);

  virtual JSObject* WrapObject(JSContext *cx, JSObject *scope) MOZ_OVERRIDE
  {
    return mozilla::dom::CSS2PropertiesBinding::Wrap(cx, scope, this);
  }

protected:
  
  
  
  virtual mozilla::css::Declaration* GetCSSDeclaration(bool aAllocate) = 0;
  virtual nsresult SetCSSDeclaration(mozilla::css::Declaration* aDecl) = 0;
  
  
  
  virtual nsIDocument* DocToUpdate() = 0;

  
  
  
  
  
  
  
  struct CSSParsingEnvironment {
    nsIURI* mSheetURI;
    nsCOMPtr<nsIURI> mBaseURI;
    nsIPrincipal* mPrincipal;
    mozilla::css::Loader* mCSSLoader;
  };
  
  
  
  
  virtual void GetCSSParsingEnvironment(CSSParsingEnvironment& aCSSParseEnv) = 0;

  
  
  static void GetCSSParsingEnvironmentForRule(mozilla::css::Rule* aRule,
                                              CSSParsingEnvironment& aCSSParseEnv);

  nsresult ParsePropertyValue(const nsCSSProperty aPropID,
                              const nsAString& aPropValue,
                              bool aIsImportant);

  
  
  nsresult RemoveProperty(const nsCSSProperty aPropID);

protected:
  virtual ~nsDOMCSSDeclaration();
  nsDOMCSSDeclaration()
  {
    SetIsDOMBinding();
  }
};

#endif 
