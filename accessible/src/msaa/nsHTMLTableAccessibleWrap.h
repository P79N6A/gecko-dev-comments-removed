







































#ifndef _NSHTMLTABLEACCESSIBLEWRAP_H
#define _NSHTMLTABLEACCESSIBLEWRAP_H

#include "nsHTMLTableAccessible.h"

#include "CAccessibleTable.h"
#include "CAccessibleTableCell.h"





class nsHTMLTableAccessibleWrap : public nsHTMLTableAccessible,
                                  public CAccessibleTable
{
public:
  nsHTMLTableAccessibleWrap(nsIContent *aContent, nsIWeakReference *aShell) :
    nsHTMLTableAccessible(aContent, aShell) {}

  
  DECL_IUNKNOWN_INHERITED

  
  NS_DECL_ISUPPORTS_INHERITED
};






class nsHTMLTableCellAccessibleWrap : public nsHTMLTableCellAccessible,
                                      public CAccessibleTableCell
{
public:
  nsHTMLTableCellAccessibleWrap(nsIContent *aContent, nsIWeakReference *aShell) :
    nsHTMLTableCellAccessible(aContent, aShell) {}

  
  DECL_IUNKNOWN_INHERITED

  
  NS_DECL_ISUPPORTS_INHERITED
};






class nsHTMLTableHeaderCellAccessibleWrap : public nsHTMLTableHeaderCellAccessible,
                                            public CAccessibleTableCell
{
public:
  nsHTMLTableHeaderCellAccessibleWrap(nsIContent *aContent,
                                      nsIWeakReference *aShell) :
    nsHTMLTableHeaderCellAccessible(aContent, aShell) {}

  
  DECL_IUNKNOWN_INHERITED

  
  NS_DECL_ISUPPORTS_INHERITED
};

#endif

