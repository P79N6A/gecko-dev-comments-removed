






































#ifndef _nsTextEquivUtils_H_
#define _nsTextEquivUtils_H_

#include "nsIAccessible.h"

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

  





  static nsresult GetNameFromSubtree(nsIAccessible *aAccessible,
                                     nsAString& aName);

  







  static nsresult GetTextEquivFromIDRefs(nsIAccessible *aAccessible,
                                         nsIAtom *aIDRefsAttr,
                                         nsAString& aTextEquiv);

  










  static nsresult AppendTextEquivFromContent(nsIAccessible *aInitiatorAcc,
                                             nsIContent *aContent,
                                             nsAString *aString);

  






  static nsresult AppendTextEquivFromTextContent(nsIContent *aContent,
                                                 nsAString *aString);

private:
  



  static nsresult AppendFromAccessibleChildren(nsIAccessible *aAccessible,
                                               nsAString *aString);
  
  



  static nsresult AppendFromAccessible(nsIAccessible *aAccessible,
                                       nsAString *aString);

  


  static nsresult AppendFromValue(nsIAccessible *aAccessible,
                                  nsAString *aString);
  


  static nsresult AppendFromDOMChildren(nsIContent *aContent,
                                        nsAString *aString);

  



  static nsresult AppendFromDOMNode(nsIContent *aContent, nsAString *aString);

  



  static PRBool AppendString(nsAString *aString,
                             const nsAString& aTextEquivalent);

  




  static PRBool IsWhitespaceString(const nsSubstring& aString);

  


  static PRBool IsWhitespace(PRUnichar aChar);

  


  static PRUint32 gRoleToNameRulesMap[];

  




  static nsCOMPtr<nsIAccessible> gInitiatorAcc;
};

#endif
