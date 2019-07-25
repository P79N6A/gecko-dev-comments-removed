






































#ifndef nsDOMCSSDeclaration_h___
#define nsDOMCSSDeclaration_h___

#include "nsICSSDeclaration.h"
#include "nsIDOMCSS2Properties.h"
#include "nsCOMPtr.h"

class nsCSSParser;
class nsIURI;
class nsIPrincipal;
class nsIDocument;

namespace mozilla {
namespace css {
class Declaration;
class Loader;
class Rule;
}
}

class nsDOMCSSDeclaration : public nsICSSDeclaration,
                            public nsIDOMCSS2Properties
{
public:
  
  
  NS_IMETHOD QueryInterface(REFNSIID aIID, void** aInstancePtr);

  NS_DECL_NSICSSDECLARATION

  
  
  NS_IMETHOD GetCssText(nsAString & aCssText);
  NS_IMETHOD SetCssText(const nsAString & aCssText);
  NS_IMETHOD GetPropertyValue(const nsAString & propertyName,
                              nsAString & _retval);
  NS_IMETHOD GetPropertyCSSValue(const nsAString & propertyName,
                                 nsIDOMCSSValue **_retval);
  NS_IMETHOD RemoveProperty(const nsAString & propertyName,
                            nsAString & _retval);
  NS_IMETHOD GetPropertyPriority(const nsAString & propertyName,
                                 nsAString & _retval);
  NS_IMETHOD SetProperty(const nsAString & propertyName,
                         const nsAString & value, const nsAString & priority);
  NS_IMETHOD GetLength(PRUint32 *aLength);
  NS_IMETHOD Item(PRUint32 index, nsAString & _retval);
  NS_IMETHOD GetParentRule(nsIDOMCSSRule * *aParentRule) = 0;

  
  
  NS_DECL_NSIDOMCSS2PROPERTIES

protected:
  
  
  
  virtual mozilla::css::Declaration* GetCSSDeclaration(PRBool aAllocate) = 0;
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
                              PRBool aIsImportant);

  
  
  nsresult RemoveProperty(const nsCSSProperty aPropID);

protected:
  virtual ~nsDOMCSSDeclaration();
};

#endif 
