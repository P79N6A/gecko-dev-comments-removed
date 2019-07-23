




































#ifndef nsTablePainter_h__
#define nsTablePainter_h__

#include "celldata.h"



#define NS_PAINT_FLAG_TABLE_BG_PAINT      0x00000001

#define NS_PAINT_FLAG_TABLE_CELL_BG_PASS  0x00000002

#include "nsIFrame.h"
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
                           nsIRenderingContext& aRenderingContext,
                           const nsRect&        aDirtyRect);

    
    ~TableBackgroundPainter();

    






    








    







    nsresult PaintTable(nsTableFrame* aTableFrame, nsMargin* aDeflate);

    






    nsresult PaintRowGroup(nsTableRowGroupFrame* aFrame)
    { return PaintRowGroup(aFrame, PR_FALSE); }

    






    nsresult PaintRow(nsTableRowFrame* aFrame)
    { return PaintRow(aFrame, PR_FALSE); }

  private:

    








    nsresult PaintTableFrame(nsTableFrame*         aTableFrame,
                             nsTableRowGroupFrame* aFirstRowGroup,
                             nsTableRowGroupFrame* aLastRowGroup,
                             nsMargin*             aDeflate = nsnull);

    



    nsresult PaintRowGroup(nsTableRowGroupFrame* aFrame,
                           PRBool                aPassThrough);
    nsresult PaintRow(nsTableRowFrame* aFrame,
                      PRBool           aPassThrough);

    




    nsresult PaintCell(nsTableCellFrame* aFrame,
                       PRBool            aPassSelf);

    




    void TranslateContext(nscoord aDX,
                          nscoord aDY);

    struct TableBackgroundData;
    friend struct TableBackgroundData;
    struct TableBackgroundData {
      nsIFrame*                 mFrame;
      
      nsRect                    mRect;
      const nsStyleBackground*  mBackground;
      const nsStyleBorder*      mBorder;

      
      PRBool IsVisible() const { return mBackground != nsnull; }

      
      TableBackgroundData();
      
      ~TableBackgroundData();
      


      void Destroy(nsPresContext* aPresContext);


      
      void Clear();

      
      void SetFull(nsIFrame* aFrame);

      
      void SetFrame(nsIFrame* aFrame);

      
      void SetData();

      
      PRBool ShouldSetBCBorder();

      
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
        mColGroup = nsnull;
      }
    };

    nsPresContext*      mPresContext;
    nsIRenderingContext& mRenderingContext;
    nsRect               mDirtyRect;
#ifdef DEBUG
    nsCompatibility      mCompatMode;
#endif
    PRBool               mIsBorderCollapse;
    Origin               mOrigin; 

    ColData*             mCols;  
    PRUint32             mNumCols;
    TableBackgroundData  mRowGroup; 
    TableBackgroundData  mRow;      
    nsRect               mCellRect; 


    nsStyleBorder        mZeroBorder;  
    nsStylePadding       mZeroPadding; 

};

#endif
