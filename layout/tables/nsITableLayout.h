



#ifndef nsITableLayout_h__
#define nsITableLayout_h__

#include "nsQueryFrame.h"
class nsIDOMElement;








class nsITableLayout
{
public:

  NS_DECL_QUERYFRAME_TARGET(nsITableLayout)

  
















  NS_IMETHOD GetCellDataAt(int32_t aRowIndex, int32_t aColIndex,
                           nsIDOMElement* &aCell,   
                           int32_t& aStartRowIndex, int32_t& aStartColIndex, 
                           int32_t& aRowSpan, int32_t& aColSpan,
                           int32_t& aActualRowSpan, int32_t& aActualColSpan,
                           bool& aIsSelected)=0;

  



  NS_IMETHOD GetTableSize(int32_t& aRowCount, int32_t& aColCount)=0;

  









  NS_IMETHOD GetIndexByRowAndColumn(int32_t aRow, int32_t aColumn,
                                    int32_t *aIndex) = 0;

  








  NS_IMETHOD GetRowAndColumnByIndex(int32_t aIndex,
                                    int32_t *aRow, int32_t *aColumn) = 0;
};

#endif
