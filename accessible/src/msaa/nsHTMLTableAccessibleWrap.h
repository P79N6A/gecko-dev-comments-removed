







































#ifndef _NSHTMLTABLEACCESSIBLEWRAP_H
#define _NSHTMLTABLEACCESSIBLEWRAP_H

#include "nsHTMLTableAccessible.h"

#include "CAccessibleTable.h"
#include "CAccessibleTableCell.h"





class nsHTMLTableAccessibleWrap : public nsHTMLTableAccessible,
                                  public CAccessibleTable
{
public:
  nsHTMLTableAccessibleWrap(nsIDOMNode* aNode, nsIWeakReference* aShell) :
    nsHTMLTableAccessible(aNode, aShell){}

  
  DECL_IUNKNOWN_INHERITED

  
  NS_DECL_ISUPPORTS_INHERITED
};






class nsHTMLTableCellAccessibleWrap : public nsHTMLTableCellAccessible,
                                      public CAccessibleTableCell
{
public:
  nsHTMLTableCellAccessibleWrap(nsIDOMNode* aNode, nsIWeakReference* aShell) :
    nsHTMLTableCellAccessible(aNode, aShell) {}

  
  DECL_IUNKNOWN_INHERITED

  
  NS_DECL_ISUPPORTS_INHERITED
};






class nsHTMLTableHeaderCellAccessibleWrap : public nsHTMLTableHeaderCellAccessible,
                                            public CAccessibleTableCell
{
public:
  nsHTMLTableHeaderCellAccessibleWrap(nsIDOMNode* aNode,
                                      nsIWeakReference* aShell) :
    nsHTMLTableHeaderCellAccessible(aNode, aShell) {}

  
  DECL_IUNKNOWN_INHERITED

  
  NS_DECL_ISUPPORTS_INHERITED
};

#endif

