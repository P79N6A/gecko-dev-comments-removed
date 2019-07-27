




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

    






    









    










    nsresult PaintTable(nsTableFrame* aTableFrame, const nsMargin& aDeflate,
                        bool aPaintTableBackground);

    






    nsresult PaintRowGroup(nsTableRowGroupFrame* aFrame)
    { return PaintRowGroup(aFrame, false); }

    






    nsresult PaintRow(nsTableRowFrame* aFrame)
    { return PaintRow(aFrame, false); }

  private:

    








    nsresult PaintTableFrame(nsTableFrame*         aTableFrame,
                             nsTableRowGroupFrame* aFirstRowGroup,
                             nsTableRowGroupFrame* aLastRowGroup,
                             const nsMargin&       aDeflate);

    



    nsresult PaintRowGroup(nsTableRowGroupFrame* aFrame,
                           bool                  aPassThrough);
    nsresult PaintRow(nsTableRowFrame* aFrame,
                      bool             aPassThrough);

    








    nsresult PaintCell(nsTableCellFrame* aCell,
                       nsRect&           aCellBGRect,
                       nsRect&           aRowBGRect,
                       nsRect&           aRowGroupBGRect,
                       nsRect&           aColBGRect,
                       bool              aPassSelf);

    







    void ComputeCellBackgrounds(nsTableCellFrame* aCell,
                                nsRect&           aCellBGRect,
                                nsRect&           aRowBGRect,
                                nsRect&           aRowGroupBGRect,
                                nsRect&           aColBGRect);

    




    void TranslateContext(nscoord aDX,
                          nscoord aDY);

    struct TableBackgroundData;
    friend struct TableBackgroundData;
    struct TableBackgroundData {
      nsIFrame*                 mFrame;
      
      nsRect                    mRect;
      bool                      mVisible;
      const nsStyleBorder*      mBorder;

      
      bool IsVisible() const { return mVisible; }

      
      TableBackgroundData();
      
      ~TableBackgroundData();
      


      void Destroy(nsPresContext* aPresContext);


      
      void Clear();

      
      void SetFull(nsIFrame* aFrame);

      
      void SetFrame(nsIFrame* aFrame);

      
      void SetData();

      
      bool ShouldSetBCBorder();

      
      nsresult SetBCBorder(nsMargin&               aBorderWidth,
                           TableBackgroundPainter* aPainter);

      private:
      nsStyleBorder* mSynthBorder;
    };

    struct ColData;
    friend struct ColData;
    struct ColData {
      TableBackgroundData  mCol;
      TableBackgroundData* mColGroup; 
      ColData() {
        mColGroup = nullptr;
      }
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

    ColData*             mCols;  
    uint32_t             mNumCols;
    TableBackgroundData  mRowGroup; 
    TableBackgroundData  mRow;      

    nsStyleBorder        mZeroBorder;  
    uint32_t             mBGPaintFlags;
};

#endif
