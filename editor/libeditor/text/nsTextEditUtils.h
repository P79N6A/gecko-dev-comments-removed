




































#ifndef nsTextEditUtils_h__
#define nsTextEditUtils_h__

#include "prtypes.h"  
#include "nsError.h"  
#include "nsString.h" 
class nsIDOMNode;
class nsIEditor;
class nsPlaintextEditor;

class nsTextEditUtils
{
public:
  
  static bool IsBody(nsIDOMNode *aNode);
  static bool IsBreak(nsIDOMNode *aNode);
  static bool IsMozBR(nsIDOMNode *aNode);
  static bool HasMozAttr(nsIDOMNode *aNode);
};





class nsAutoEditInitRulesTrigger
{
  private:
    nsPlaintextEditor *mEd;
    nsresult &mRes;
  public:
  nsAutoEditInitRulesTrigger( nsPlaintextEditor *aEd, nsresult &aRes);
  ~nsAutoEditInitRulesTrigger();
};


#endif 

