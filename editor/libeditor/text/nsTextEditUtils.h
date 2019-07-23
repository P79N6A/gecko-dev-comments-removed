




































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
  
  static PRBool IsBody(nsIDOMNode *aNode);
  static PRBool IsBreak(nsIDOMNode *aNode);
  static PRBool IsMozBR(nsIDOMNode *aNode);
  static PRBool HasMozAttr(nsIDOMNode *aNode);
  static PRBool InBody(nsIDOMNode *aNode, nsIEditor *aEditor);
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

