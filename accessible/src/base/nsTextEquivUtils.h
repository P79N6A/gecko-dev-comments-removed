






#ifndef _nsTextEquivUtils_H_
#define _nsTextEquivUtils_H_

#include "Accessible.h"
#include "Role.h"

#include "nsIContent.h"
#include "nsIStringBundle.h"




enum ETextEquivRule
{
  
  eNoRule = 0x00,

  
  
  eFromSubtreeIfRec = 0x01,

  
  eFromSubtree = 0x03,

  
  
  
  eFromValue = 0x04
};





class nsTextEquivUtils
{
public:

  





  static nsresult GetNameFromSubtree(Accessible* aAccessible,
                                     nsAString& aName);

  







  static nsresult GetTextEquivFromIDRefs(Accessible* aAccessible,
                                         nsIAtom *aIDRefsAttr,
                                         nsAString& aTextEquiv);

  










  static nsresult AppendTextEquivFromContent(Accessible* aInitiatorAcc,
                                             nsIContent *aContent,
                                             nsAString *aString);

  






  static nsresult AppendTextEquivFromTextContent(nsIContent *aContent,
                                                 nsAString *aString);

private:
  



  static nsresult AppendFromAccessibleChildren(Accessible* aAccessible,
                                               nsAString *aString);
  
  



  static nsresult AppendFromAccessible(Accessible* aAccessible,
                                       nsAString *aString);

  


  static nsresult AppendFromValue(Accessible* aAccessible,
                                  nsAString *aString);
  


  static nsresult AppendFromDOMChildren(nsIContent *aContent,
                                        nsAString *aString);

  



  static nsresult AppendFromDOMNode(nsIContent *aContent, nsAString *aString);

  



  static bool AppendString(nsAString *aString,
                             const nsAString& aTextEquivalent);

  




  static bool IsWhitespaceString(const nsSubstring& aString);

  


  static bool IsWhitespace(PRUnichar aChar);

  


  static PRUint32 GetRoleRule(mozilla::a11y::roles::Role aRole);

  




  static nsRefPtr<Accessible> gInitiatorAcc;
};

#endif
