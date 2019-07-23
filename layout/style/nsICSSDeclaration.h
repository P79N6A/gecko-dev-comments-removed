









































#ifndef nsICSSDeclaration_h__
#define nsICSSDeclaration_h__









#include "nsIDOMCSSStyleDeclaration.h"
#include "nsCSSProperty.h"


#define NS_ICSSDECLARATION_IID \
 { 0x57eb81d1, 0xa607, 0x4429, \
    {0x92, 0x6b, 0x80, 0x25, 0x19, 0xd4, 0x3a, 0xad } }

class nsINode;

class nsICSSDeclaration : public nsIDOMCSSStyleDeclaration
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ICSSDECLARATION_IID)

  



  NS_IMETHOD GetPropertyValue(const nsCSSProperty aPropID,
                              nsAString& aValue) = 0;

  
  
  NS_IMETHOD GetPropertyValue(const nsAString& aPropName,
                              nsAString& aValue) = 0;
  
  




  NS_IMETHOD SetPropertyValue(const nsCSSProperty aPropID,
                              const nsAString& aValue) = 0;

  virtual nsINode *GetParentObject() = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsICSSDeclaration, NS_ICSSDECLARATION_IID)

#define NS_DECL_NSICSSDECLARATION                               \
  NS_IMETHOD GetPropertyValue(const nsCSSProperty aPropID,    \
                              nsAString& aValue);               \
  NS_IMETHOD SetPropertyValue(const nsCSSProperty aPropID,    \
                              const nsAString& aValue);

  

#endif 
