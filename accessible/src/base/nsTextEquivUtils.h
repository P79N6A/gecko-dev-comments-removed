






#ifndef _nsTextEquivUtils_H_
#define _nsTextEquivUtils_H_

#include "Accessible.h"
#include "nsIStringBundle.h"
#include "Role.h"

class nsIContent;




enum ETextEquivRule
{
  
  eNoNameRule = 0x00,

  
  
  eNameFromSubtreeIfReqRule = 0x01,

  
  eNameFromSubtreeRule = 0x03,

  
  
  
  eNameFromValueRule = 0x04
};





class nsTextEquivUtils
{
public:
  typedef mozilla::a11y::Accessible Accessible;

  





  static nsresult GetNameFromSubtree(Accessible* aAccessible,
                                     nsAString& aName);

  




  static void GetTextEquivFromSubtree(Accessible* aAccessible,
                                      nsString& aTextEquiv);

  







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

  


  static uint32_t GetRoleRule(mozilla::a11y::roles::Role aRole);

  




  static nsRefPtr<Accessible> gInitiatorAcc;
};

#endif
