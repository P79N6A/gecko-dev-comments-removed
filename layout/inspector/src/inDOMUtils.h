




































#ifndef __inDOMUtils_h__
#define __inDOMUtils_h__

#include "inIDOMUtils.h"

#include "nsIEventStateManager.h"
#include "nsISupportsArray.h"
#include "nsIInspectorCSSUtils.h"

class inDOMUtils : public inIDOMUtils
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_INIDOMUTILS

  inDOMUtils();
  virtual ~inDOMUtils();

protected:
  nsCOMPtr<nsIInspectorCSSUtils> mCSSUtils;

};


#define IN_DOMUTILS_CID \
{ 0x40b22006, 0x5dd5, 0x42f2, { 0xbf, 0xe7, 0x7d, 0xbf, 0x7, 0x57, 0xab, 0x8b } }

#endif 
