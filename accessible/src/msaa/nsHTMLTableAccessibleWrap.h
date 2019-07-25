






#ifndef _NSHTMLTABLEACCESSIBLEWRAP_H
#define _NSHTMLTABLEACCESSIBLEWRAP_H

#include "nsHTMLTableAccessible.h"

#include "CAccessibleTable.h"
#include "CAccessibleTableCell.h"





class nsHTMLTableAccessibleWrap : public nsHTMLTableAccessible,
                                  public CAccessibleTable
{
public:
  nsHTMLTableAccessibleWrap(nsIContent* aContent, DocAccessible* aDoc) :
    nsHTMLTableAccessible(aContent, aDoc) {}

  
  DECL_IUNKNOWN_INHERITED

  
  NS_DECL_ISUPPORTS_INHERITED
};






class nsHTMLTableCellAccessibleWrap : public nsHTMLTableCellAccessible,
                                      public CAccessibleTableCell
{
public:
  nsHTMLTableCellAccessibleWrap(nsIContent* aContent, DocAccessible* aDoc) :
    nsHTMLTableCellAccessible(aContent, aDoc) {}

  
  DECL_IUNKNOWN_INHERITED

  
  NS_DECL_ISUPPORTS_INHERITED
};






class nsHTMLTableHeaderCellAccessibleWrap : public nsHTMLTableHeaderCellAccessible,
                                            public CAccessibleTableCell
{
public:
  nsHTMLTableHeaderCellAccessibleWrap(nsIContent* aContent,
                                      DocAccessible* aDoc) :
    nsHTMLTableHeaderCellAccessible(aContent, aDoc) {}

  
  DECL_IUNKNOWN_INHERITED

  
  NS_DECL_ISUPPORTS_INHERITED
};

#endif

