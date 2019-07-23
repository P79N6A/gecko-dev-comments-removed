



































#ifndef nsITableCellLayout_h__
#define nsITableCellLayout_h__

#include "nsQueryFrame.h"







class nsITableCellLayout
{
public:

  NS_DECL_QUERYFRAME_TARGET(nsITableCellLayout)

  
  NS_IMETHOD GetCellIndexes(PRInt32 &aRowIndex, PRInt32 &aColIndex)=0;

  
  virtual nsresult GetRowIndex(PRInt32 &aRowIndex) const = 0;
  
  
  virtual nsresult GetColIndex(PRInt32 &aColIndex) const = 0;
};

#endif



