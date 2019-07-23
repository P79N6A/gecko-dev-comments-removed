






































#ifndef nsDOMCSSDeclaration_h___
#define nsDOMCSSDeclaration_h___

#include "nsICSSDeclaration.h"
#include "nsIDOMNSCSS2Properties.h"
#include "nsCycleCollectionParticipant.h"
#include "nsCOMPtr.h"

class nsCSSDeclaration;
class nsICSSParser;
class nsICSSLoader;
class nsIURI;
class nsIPrincipal;
class nsIDocument;

class CSS2PropertiesTearoff : public nsIDOMNSCSS2Properties
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS(CSS2PropertiesTearoff)

  NS_DECL_NSIDOMCSS2PROPERTIES
  NS_DECL_NSIDOMNSCSS2PROPERTIES

  CSS2PropertiesTearoff(nsICSSDeclaration *aOuter);
  virtual ~CSS2PropertiesTearoff();

private:
  nsCOMPtr<nsICSSDeclaration> mOuter;
};

class nsDOMCSSDeclaration : public nsICSSDeclaration
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

protected:
  
  
  
  virtual nsresult GetCSSDeclaration(nsCSSDeclaration **aDecl,
                                     PRBool aAllocate) = 0;
  virtual nsresult DeclarationChanged() = 0;
  
  
  
  virtual nsIDocument* DocToUpdate() = 0;
  
  
  
  
  virtual nsresult GetCSSParsingEnvironment(nsIURI** aSheetURI,
                                            nsIURI** aBaseURI,
                                            nsIPrincipal** aSheetPrincipal,
                                            nsICSSLoader** aCSSLoader,
                                            nsICSSParser** aCSSParser) = 0;

  nsresult ParsePropertyValue(const nsCSSProperty aPropID,
                              const nsAString& aPropValue);
  nsresult ParseDeclaration(const nsAString& aDecl,
                            PRBool aParseOnlyOneDecl, PRBool aClearOldDecl);

  
  
  nsresult RemoveProperty(const nsCSSProperty aPropID);
  
  
protected:
  virtual ~nsDOMCSSDeclaration();
};

#endif 
