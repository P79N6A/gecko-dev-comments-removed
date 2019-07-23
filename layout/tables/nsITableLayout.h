



































#ifndef nsITableLayout_h__
#define nsITableLayout_h__

#include "nsISupports.h"
class nsIDOMElement;



#define NS_ITABLELAYOUT_IID \
 { 0xa9222e6b, 0x437e, 0x11d3, { 0xb2, 0x27, 0x0, 0x40, 0x95, 0xe2, 0x7a, 0x10 }}








class nsITableLayout : public nsISupports
{
public:

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ITABLELAYOUT_IID)

  
















  NS_IMETHOD GetCellDataAt(PRInt32 aRowIndex, PRInt32 aColIndex,
                           nsIDOMElement* &aCell,   
                           PRInt32& aStartRowIndex, PRInt32& aStartColIndex, 
                           PRInt32& aRowSpan, PRInt32& aColSpan,
                           PRInt32& aActualRowSpan, PRInt32& aActualColSpan,
                           PRBool& aIsSelected)=0;

  



  NS_IMETHOD GetTableSize(PRInt32& aRowCount, PRInt32& aColCount)=0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsITableLayout, NS_ITABLELAYOUT_IID)

#endif
