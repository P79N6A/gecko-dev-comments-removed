






#ifndef nsDOMCSSDeclaration_h___
#define nsDOMCSSDeclaration_h___

#include "nsICSSDeclaration.h"

#include "mozilla/Attributes.h"
#include "nsIURI.h"
#include "nsCOMPtr.h"

class nsIPrincipal;
class nsIDocument;
struct JSContext;
class JSObject;

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
  
  
  NS_IMETHOD QueryInterface(REFNSIID aIID, void** aInstancePtr) override;

  
  
  
  NS_IMETHOD_(MozExternalRefCountType) AddRef() override = 0;
  NS_IMETHOD_(MozExternalRefCountType) Release() override = 0;

  NS_DECL_NSICSSDECLARATION
  using nsICSSDeclaration::GetLength;

  
  
  NS_IMETHOD GetCssText(nsAString & aCssText) override;
  NS_IMETHOD SetCssText(const nsAString & aCssText) override;
  NS_IMETHOD GetPropertyValue(const nsAString & propertyName,
                              nsAString & _retval) override;
  virtual already_AddRefed<mozilla::dom::CSSValue>
    GetPropertyCSSValue(const nsAString & propertyName,
                        mozilla::ErrorResult& aRv) override;
  using nsICSSDeclaration::GetPropertyCSSValue;
  NS_IMETHOD RemoveProperty(const nsAString & propertyName,
                            nsAString & _retval) override;
  NS_IMETHOD GetPropertyPriority(const nsAString & propertyName,
                                 nsAString & _retval) override;
  NS_IMETHOD SetProperty(const nsAString & propertyName,
                         const nsAString & value, const nsAString & priority) override;
  NS_IMETHOD GetLength(uint32_t *aLength) override;
  NS_IMETHOD GetParentRule(nsIDOMCSSRule * *aParentRule) override = 0;

  
#define CSS_PROP_PUBLIC_OR_PRIVATE(publicname_, privatename_) publicname_
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
#define CSS_PROP_LIST_INCLUDE_LOGICAL
#define CSS_PROP_SHORTHAND(name_, id_, method_, flags_, pref_)  \
  CSS_PROP(name_, id_, method_, flags_, pref_, X, X, X, X, X)
#include "nsCSSPropList.h"

#define CSS_PROP_ALIAS(aliasname_, propid_, aliasmethod_, pref_)  \
  CSS_PROP(X, propid_, aliasmethod_, X, pref_, X, X, X, X, X)
#include "nsCSSPropAliasList.h"
#undef CSS_PROP_ALIAS

#undef CSS_PROP_SHORTHAND
#undef CSS_PROP_LIST_INCLUDE_LOGICAL
#undef CSS_PROP_LIST_EXCLUDE_INTERNAL
#undef CSS_PROP
#undef CSS_PROP_PUBLIC_OR_PRIVATE

  virtual void IndexedGetter(uint32_t aIndex, bool& aFound, nsAString& aPropName) override;

  virtual JSObject* WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override;

protected:
  
  enum Operation {
    
    
    
    eOperation_Read,

    
    
    
    
    eOperation_Modify,

    
    
    
    
    eOperation_RemoveProperty
  };
  virtual mozilla::css::Declaration* GetCSSDeclaration(Operation aOperation) = 0;
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

  void GetCustomPropertyValue(const nsAString& aPropertyName, nsAString& aValue);
  nsresult RemoveCustomProperty(const nsAString& aPropertyName);
  nsresult ParseCustomPropertyValue(const nsAString& aPropertyName,
                                    const nsAString& aPropValue,
                                    bool aIsImportant);

protected:
  virtual ~nsDOMCSSDeclaration();
};

bool IsCSSPropertyExposedToJS(nsCSSProperty aProperty, JSContext* cx, JSObject* obj);

template <nsCSSProperty Property>
MOZ_ALWAYS_INLINE bool IsCSSPropertyExposedToJS(JSContext* cx, JSObject* obj)
{
  return IsCSSPropertyExposedToJS(Property, cx, obj);
}

#endif
