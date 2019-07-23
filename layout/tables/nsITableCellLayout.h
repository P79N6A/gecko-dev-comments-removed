



































#ifndef nsITableCellLayout_h__
#define nsITableCellLayout_h__

#include "nsISupports.h"



#define NS_ITABLECELLAYOUT_IID \
{ 0x0238f187, 0x033d, 0x426b, \
 { 0xbd, 0x03, 0x96, 0xeb, 0x75, 0xaf, 0x51, 0x29 } }







class nsITableCellLayout : public nsISupports
{
public:

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ITABLECELLAYOUT_IID)

  
  NS_IMETHOD GetCellIndexes(PRInt32 &aRowIndex, PRInt32 &aColIndex)=0;

  
  virtual nsresult GetRowIndex(PRInt32 &aRowIndex) const = 0;
  
  
  virtual nsresult GetColIndex(PRInt32 &aColIndex) const = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsITableCellLayout, NS_ITABLECELLAYOUT_IID)

#endif



