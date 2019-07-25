






































#ifndef nsDOMCSSDeclaration_h___
#define nsDOMCSSDeclaration_h___

#include "nsICSSDeclaration.h"
#include "nsIDOMNSCSS2Properties.h"
#include "nsCOMPtr.h"

class nsCSSParser;
class nsIURI;
class nsIPrincipal;
class nsIDocument;

namespace mozilla {
namespace css {
class Declaration;
class Loader;
}
}

class nsDOMCSSDeclaration : public nsICSSDeclaration,
                            public nsIDOMNSCSS2Properties
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
  NS_DECL_NSIDOMSVGCSS2PROPERTIES
  NS_DECL_NSIDOMNSCSS2PROPERTIES

protected:
  
  
  
  virtual mozilla::css::Declaration* GetCSSDeclaration(PRBool aAllocate) = 0;
  virtual nsresult DeclarationChanged() = 0;
  
  
  
  virtual nsIDocument* DocToUpdate() = 0;

  
  
  
  virtual nsresult GetCSSParsingEnvironment(nsIURI** aSheetURI,
                                            nsIURI** aBaseURI,
                                            nsIPrincipal** aSheetPrincipal,
                                            mozilla::css::Loader** aCSSLoader) = 0;

  nsresult ParsePropertyValue(const nsCSSProperty aPropID,
                              const nsAString& aPropValue,
                              PRBool aIsImportant);

  
  
  nsresult RemoveProperty(const nsCSSProperty aPropID);

protected:
  virtual ~nsDOMCSSDeclaration();
};

#endif 
