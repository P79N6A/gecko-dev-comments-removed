



#ifndef nsITableCellLayout_h__
#define nsITableCellLayout_h__

#include "nsQueryFrame.h"







class nsITableCellLayout
{
public:

  NS_DECL_QUERYFRAME_TARGET(nsITableCellLayout)

  
  NS_IMETHOD GetCellIndexes(int32_t &aRowIndex, int32_t &aColIndex)=0;

  
  virtual nsresult GetRowIndex(int32_t &aRowIndex) const = 0;
  
  
  virtual nsresult GetColIndex(int32_t &aColIndex) const = 0;
};

#endif



