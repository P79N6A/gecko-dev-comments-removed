




































#ifndef __inDOMUtils_h__
#define __inDOMUtils_h__

#include "inIDOMUtils.h"

#include "nsISupportsArray.h"

class nsRuleNode;
class nsStyleContext;
class nsIAtom;
class nsIContent;

class inDOMUtils : public inIDOMUtils
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_INIDOMUTILS

  inDOMUtils();
  virtual ~inDOMUtils();

private:
  
  static nsresult GetRuleNodeForContent(nsIContent* aContent,
                                        nsIAtom* aPseudo,
                                        nsStyleContext** aStyleContext,
                                        nsRuleNode** aRuleNode);
};


#define IN_DOMUTILS_CID \
  {0x0a499822, 0xa287, 0x4089, {0xad, 0x3f, 0x9f, 0xfc, 0xd4, 0xf4, 0x02, 0x63}}

#endif 
