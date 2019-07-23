



































#ifndef nsITableLayout_h__
#define nsITableLayout_h__

#include "nsISupports.h"
class nsIDOMElement;



#define NS_ITABLELAYOUT_IID \
 { 0xf8363dea, 0x11ad, 0x483a, { 0xba, 0xea, 0xf6, 0xf2, 0xc3, 0x58, 0x8d, 0xde }}








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

  









  NS_IMETHOD GetIndexByRowAndColumn(PRInt32 aRow, PRInt32 aColumn,
                                    PRInt32 *aIndex) = 0;

  








  NS_IMETHOD GetRowAndColumnByIndex(PRInt32 aIndex,
                                    PRInt32 *aRow, PRInt32 *aColumn) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsITableLayout, NS_ITABLELAYOUT_IID)

#endif
