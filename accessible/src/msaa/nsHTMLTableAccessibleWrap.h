







































#ifndef _NSHTMLTABLEACCESSIBLEWRAP_H
#define _NSHTMLTABLEACCESSIBLEWRAP_H

#include "nsHTMLTableAccessible.h"
#include "CAccessibleTable.h"

class nsHTMLTableAccessibleWrap : public nsHTMLTableAccessible,
                                  public CAccessibleTable
{
public:
  nsHTMLTableAccessibleWrap(nsIDOMNode* aNode, nsIWeakReference* aShell) :
    nsHTMLTableAccessible(aNode, aShell){}

  
  DECL_IUNKNOWN_INHERITED

  
  NS_DECL_ISUPPORTS_INHERITED
};

class nsHTMLTableHeadAccessibleWrap : public nsHTMLTableHeadAccessible,
                                      public CAccessibleTable
{
public:
  nsHTMLTableHeadAccessibleWrap(nsIDOMNode* aNode, nsIWeakReference* aShell) :
    nsHTMLTableHeadAccessible(aNode, aShell){}

  
  DECL_IUNKNOWN_INHERITED

  
  NS_DECL_ISUPPORTS_INHERITED
};

#endif

