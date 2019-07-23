







































#ifndef _NSARIAGRIDACCESSIBLEWRAP_H
#define _NSARIAGRIDACCESSIBLEWRAP_H

#include "nsARIAGridAccessible.h"
#include "CAccessibleTable.h"





class nsARIAGridAccessibleWrap : public nsARIAGridAccessible,
                                 public CAccessibleTable
{
public:
  nsARIAGridAccessibleWrap(nsIDOMNode* aNode, nsIWeakReference* aShell) :
    nsARIAGridAccessible(aNode, aShell) {}

  
  DECL_IUNKNOWN_INHERITED

  
  NS_DECL_ISUPPORTS_INHERITED
};

#endif
