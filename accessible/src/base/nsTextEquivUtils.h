






































#ifndef _nsTextEquivUtils_H_
#define _nsTextEquivUtils_H_

#include "nsAccessible.h"

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

  





  static nsresult GetNameFromSubtree(nsAccessible *aAccessible,
                                     nsAString& aName);

  







  static nsresult GetTextEquivFromIDRefs(nsAccessible *aAccessible,
                                         nsIAtom *aIDRefsAttr,
                                         nsAString& aTextEquiv);

  










  static nsresult AppendTextEquivFromContent(nsAccessible *aInitiatorAcc,
                                             nsIContent *aContent,
                                             nsAString *aString);

  






  static nsresult AppendTextEquivFromTextContent(nsIContent *aContent,
                                                 nsAString *aString);

private:
  



  static nsresult AppendFromAccessibleChildren(nsAccessible *aAccessible,
                                               nsAString *aString);
  
  



  static nsresult AppendFromAccessible(nsAccessible *aAccessible,
                                       nsAString *aString);

  


  static nsresult AppendFromValue(nsAccessible *aAccessible,
                                  nsAString *aString);
  


  static nsresult AppendFromDOMChildren(nsIContent *aContent,
                                        nsAString *aString);

  



  static nsresult AppendFromDOMNode(nsIContent *aContent, nsAString *aString);

  



  static bool AppendString(nsAString *aString,
                             const nsAString& aTextEquivalent);

  




  static bool IsWhitespaceString(const nsSubstring& aString);

  


  static bool IsWhitespace(PRUnichar aChar);

  


  static PRUint32 gRoleToNameRulesMap[];

  




  static nsRefPtr<nsAccessible> gInitiatorAcc;
};

#endif
