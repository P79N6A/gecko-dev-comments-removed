




#ifndef nsTablePainter_h__
#define nsTablePainter_h__

#include "celldata.h"



#define NS_PAINT_FLAG_TABLE_BG_PAINT      0x00000001

#define NS_PAINT_FLAG_TABLE_CELL_BG_PASS  0x00000002

class nsIFrame;
class nsTableFrame;
class nsTableRowGroupFrame;
class nsTableRowFrame;
class nsTableCellFrame;

class TableBackgroundPainter
{
  




  public:

    enum Origin { eOrigin_Table, eOrigin_TableRowGroup, eOrigin_TableRow };

    










    TableBackgroundPainter(nsTableFrame*        aTableFrame,
                           Origin               aOrigin,
                           nsPresContext*       aPresContext,
                           nsRenderingContext& aRenderingContext,
                           const nsRect&        aDirtyRect,
                           const nsPoint&       aPt,
                           uint32_t             aBGPaintFlags);

    
    ~TableBackgroundPainter();

    






    









    










    void PaintTable(nsTableFrame* aTableFrame, const nsMargin& aDeflate,
                    bool aPaintTableBackground);

    






    void PaintRowGroup(nsTableRowGroupFrame* aFrame);

    






    void PaintRow(nsTableRowFrame* aFrame);

  private:
    struct TableBackgroundData;

    








    void PaintTableFrame(nsTableFrame*         aTableFrame,
                         nsTableRowGroupFrame* aFirstRowGroup,
                         nsTableRowGroupFrame* aLastRowGroup,
                         const nsMargin&       aDeflate);

    





    void PaintRowGroup(nsTableRowGroupFrame* aFrame,
                       TableBackgroundData   aRowGroupBGData,
                       bool                  aPassThrough);

    void PaintRow(nsTableRowFrame* aFrame,
                  const TableBackgroundData& aRowGroupBGData,
                  TableBackgroundData aRowBGData,
                  bool             aPassThrough);

    










    void PaintCell(nsTableCellFrame* aCell,
                   const TableBackgroundData& aRowGroupBGData,
                   const TableBackgroundData& aRowBGData,
                   nsRect&           aCellBGRect,
                   nsRect&           aRowBGRect,
                   nsRect&           aRowGroupBGRect,
                   nsRect&           aColBGRect,
                   bool              aPassSelf);

    









    void ComputeCellBackgrounds(nsTableCellFrame* aCell,
                                const TableBackgroundData& aRowGroupBGData,
                                const TableBackgroundData& aRowBGData,
                                nsRect&           aCellBGRect,
                                nsRect&           aRowBGRect,
                                nsRect&           aRowGroupBGRect,
                                nsRect&           aColBGRect);

    




    void TranslateContext(nscoord aDX,
                          nscoord aDY);

    struct TableBackgroundData {
    public:
      


      TableBackgroundData();

      



      explicit TableBackgroundData(nsIFrame* aFrame);

      
      ~TableBackgroundData() {}

      
      bool IsVisible() const { return mVisible; }

      
      void MakeInvisible() { mVisible = false; }

      
      bool ShouldSetBCBorder() const;

      
      void SetBCBorder(const nsMargin& aBorderWidth);

      






      nsStyleBorder StyleBorder(const nsStyleBorder& aZeroBorder) const;

      nsIFrame* const mFrame;

      
      nsRect mRect;

    private:
      nsMargin mSynthBorderWidths;
      bool mVisible;
      bool mUsesSynthBorder;
    };

    struct ColData {
      ColData(nsIFrame* aFrame, TableBackgroundData& aColGroupBGData);
      TableBackgroundData mCol;
      TableBackgroundData& mColGroup; 
    };

    nsPresContext*      mPresContext;
    nsRenderingContext& mRenderingContext;
    nsPoint              mRenderPt;
    nsRect               mDirtyRect;
#ifdef DEBUG
    nsCompatibility      mCompatMode;
#endif
    bool                 mIsBorderCollapse;
    Origin               mOrigin; 

    nsTArray<TableBackgroundData> mColGroups;
    nsTArray<ColData>    mCols;
    size_t               mNumCols;

    nsStyleBorder        mZeroBorder;  
    uint32_t             mBGPaintFlags;
};

#endif
