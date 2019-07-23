



































#ifndef nsITableLayout_h__
#define nsITableLayout_h__

#include "nsQueryFrame.h"
class nsIDOMElement;








class nsITableLayout
{
public:

  NS_DECLARE_FRAME_ACCESSOR(nsITableLayout)

  
















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

#endif
