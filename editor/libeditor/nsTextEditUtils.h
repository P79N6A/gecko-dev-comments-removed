




#ifndef nsTextEditUtils_h__
#define nsTextEditUtils_h__

#include "nscore.h"

namespace mozilla {
namespace dom {
class Element;
} 
} 

class nsIDOMNode;
class nsPlaintextEditor;

class nsTextEditUtils
{
public:
  
  static bool IsBody(nsIDOMNode* aNode);
  static bool IsBreak(nsIDOMNode* aNode);
  static bool IsBreak(nsINode* aNode);
  static bool IsMozBR(nsIDOMNode* aNode);
  static bool IsMozBR(nsINode* aNode);
  static bool HasMozAttr(nsIDOMNode* aNode);
};





class nsAutoEditInitRulesTrigger
{
private:
  nsPlaintextEditor* mEd;
  nsresult& mRes;
public:
  nsAutoEditInitRulesTrigger(nsPlaintextEditor* aEd, nsresult& aRes);
  ~nsAutoEditInitRulesTrigger();
};

#endif 
