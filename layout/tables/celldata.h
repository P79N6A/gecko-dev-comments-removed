



































#ifndef CellData_h__
#define CellData_h__

#include "nsISupports.h"
#include "nsCoord.h"
#include "mozilla/gfx/Types.h"

class nsTableCellFrame;
class nsCellMap;
class BCCellData;


#define MAX_ROWSPAN 8190 // the cellmap can not handle more
#define MAX_COLSPAN 1000 // limit as IE and opera do




class CellData
{
public:
  


  void   Init(nsTableCellFrame* aCellFrame);

  


  PRBool IsOrig() const;

  




  PRBool IsDead() const;

  


  PRBool IsSpan() const;

  


  PRBool IsRowSpan() const;

  




  PRBool IsZeroRowSpan() const;

  


  void SetZeroRowSpan(PRBool aIsZero);

  


  PRUint32 GetRowSpanOffset() const;

  


  void SetRowSpanOffset(PRUint32 aSpan);

  


  PRBool IsColSpan() const;

  





  PRBool IsZeroColSpan() const;

  


  void SetZeroColSpan(PRBool aIsZero);

  


  PRUint32 GetColSpanOffset() const;

  


  void SetColSpanOffset(PRUint32 aSpan);

  


  PRBool IsOverlap() const;

  


  void SetOverlap(PRBool aOverlap);

  



  nsTableCellFrame* GetCellFrame() const;

private:
  friend class nsCellMap;
  friend class BCCellData;

  


  CellData(nsTableCellFrame* aOrigCell);  

  
  ~CellData(); 

protected:

  
  
  
  
  
  
  union {
    nsTableCellFrame* mOrigCell;
    unsigned long     mBits;
  };
};


enum BCBorderOwner
{
  eTableOwner        =  0,
  eColGroupOwner     =  1,
  eAjaColGroupOwner  =  2, 
  eColOwner          =  3,
  eAjaColOwner       =  4, 
  eRowGroupOwner     =  5,
  eAjaRowGroupOwner  =  6, 
  eRowOwner          =  7,
  eAjaRowOwner       =  8, 
  eCellOwner         =  9,
  eAjaCellOwner      = 10  
};

typedef PRUint16 BCPixelSize;



#define MAX_BORDER_WIDTH nscoord(PR_BITMASK(sizeof(BCPixelSize) * 8))

static inline nscoord
BC_BORDER_TOP_HALF_COORD(PRInt32 p2t, PRUint16 px)    { return (px - px / 2) * p2t; }
static inline nscoord
BC_BORDER_RIGHT_HALF_COORD(PRInt32 p2t, PRUint16 px)  { return (     px / 2) * p2t; }
static inline nscoord
BC_BORDER_BOTTOM_HALF_COORD(PRInt32 p2t, PRUint16 px) { return (     px / 2) * p2t; }
static inline nscoord
BC_BORDER_LEFT_HALF_COORD(PRInt32 p2t, PRUint16 px)   { return (px - px / 2) * p2t; }

#define BC_BORDER_TOP_HALF(px)    ((px) - (px) / 2)
#define BC_BORDER_RIGHT_HALF(px)  ((px) / 2)
#define BC_BORDER_BOTTOM_HALF(px) ((px) / 2)
#define BC_BORDER_LEFT_HALF(px)   ((px) - (px) / 2)


class BCData
{
public:
  BCData();

  ~BCData();

  nscoord GetLeftEdge(BCBorderOwner& aOwner,
                      PRBool&        aStart) const;

  void SetLeftEdge(BCBorderOwner aOwner,
                   nscoord       aSize,
                   PRBool        aStart);

  nscoord GetTopEdge(BCBorderOwner& aOwner,
                     PRBool&        aStart) const;

  void SetTopEdge(BCBorderOwner aOwner,
                  nscoord       aSize,
                  PRBool        aStart);

  BCPixelSize GetCorner(mozilla::css::Side&       aCornerOwner,
                        PRPackedBool&  aBevel) const;

  void SetCorner(BCPixelSize aSubSize,
                 mozilla::css::Side aOwner,
                 PRBool  aBevel);

  PRBool IsLeftStart() const;

  void SetLeftStart(PRBool aValue);

  PRBool IsTopStart() const;

  void SetTopStart(PRBool aValue);


protected:
  BCPixelSize mLeftSize;      
  BCPixelSize mTopSize;       
  BCPixelSize mCornerSubSize; 
                              
                              
                              
                              
  unsigned mLeftOwner:     4; 
  unsigned mTopOwner:      4; 
  unsigned mLeftStart:     1; 
  unsigned mTopStart:      1; 
  unsigned mCornerSide:    2; 
  unsigned mCornerBevel:   1; 
};





class BCCellData : public CellData
{
public:
  BCCellData(nsTableCellFrame* aOrigCell);
  ~BCCellData();

  BCData mData;
};


#define SPAN             0x00000001 // there a row or col span
#define ROW_SPAN         0x00000002 // there is a row span
#define ROW_SPAN_0       0x00000004 // the row span is 0
#define ROW_SPAN_OFFSET  0x0000FFF8 // the row offset to the data containing the original cell
#define COL_SPAN         0x00010000 // there is a col span
#define COL_SPAN_0       0x00020000 // the col span is 0
#define OVERLAP          0x00040000 // there is a row span and col span but no by same cell
#define COL_SPAN_OFFSET  0xFFF80000 // the col offset to the data containing the original cell
#define ROW_SPAN_SHIFT   3          // num bits to shift to get right justified row span
#define COL_SPAN_SHIFT   19         // num bits to shift to get right justified col span

inline nsTableCellFrame* CellData::GetCellFrame() const
{
  if (SPAN != (SPAN & mBits)) {
    return mOrigCell;
  }
  return nsnull;
}

inline void CellData::Init(nsTableCellFrame* aCellFrame)
{
  mOrigCell = aCellFrame;
}

inline PRBool CellData::IsOrig() const
{
  return ((nsnull != mOrigCell) && (SPAN != (SPAN & mBits)));
}

inline PRBool CellData::IsDead() const
{
  return (0 == mBits);
}

inline PRBool CellData::IsSpan() const
{
  return (SPAN == (SPAN & mBits));
}

inline PRBool CellData::IsRowSpan() const
{
  return (SPAN     == (SPAN & mBits)) &&
         (ROW_SPAN == (ROW_SPAN & mBits));
}

inline PRBool CellData::IsZeroRowSpan() const
{
  return (SPAN       == (SPAN & mBits))     &&
         (ROW_SPAN   == (ROW_SPAN & mBits)) &&
         (ROW_SPAN_0 == (ROW_SPAN_0 & mBits));
}

inline void CellData::SetZeroRowSpan(PRBool aIsZeroSpan)
{
  if (SPAN == (SPAN & mBits)) {
    if (aIsZeroSpan) {
      mBits |= ROW_SPAN_0;
    }
    else {
      mBits &= ~ROW_SPAN_0;
    }
  }
}

inline PRUint32 CellData::GetRowSpanOffset() const
{
  if ((SPAN == (SPAN & mBits)) && ((ROW_SPAN == (ROW_SPAN & mBits)))) {
    return (PRUint32)((mBits & ROW_SPAN_OFFSET) >> ROW_SPAN_SHIFT);
  }
  return 0;
}

inline void CellData::SetRowSpanOffset(PRUint32 aSpan)
{
  mBits &= ~ROW_SPAN_OFFSET;
  mBits |= (aSpan << ROW_SPAN_SHIFT);
  mBits |= SPAN;
  mBits |= ROW_SPAN;
}

inline PRBool CellData::IsColSpan() const
{
  return (SPAN     == (SPAN & mBits)) &&
         (COL_SPAN == (COL_SPAN & mBits));
}

inline PRBool CellData::IsZeroColSpan() const
{
  return (SPAN       == (SPAN & mBits))     &&
         (COL_SPAN   == (COL_SPAN & mBits)) &&
         (COL_SPAN_0 == (COL_SPAN_0 & mBits));
}

inline void CellData::SetZeroColSpan(PRBool aIsZeroSpan)
{
  if (SPAN == (SPAN & mBits)) {
    if (aIsZeroSpan) {
      mBits |= COL_SPAN_0;
    }
    else {
      mBits &= ~COL_SPAN_0;
    }
  }
}

inline PRUint32 CellData::GetColSpanOffset() const
{
  if ((SPAN == (SPAN & mBits)) && ((COL_SPAN == (COL_SPAN & mBits)))) {
    return (PRUint32)((mBits & COL_SPAN_OFFSET) >> COL_SPAN_SHIFT);
  }
  return 0;
}

inline void CellData::SetColSpanOffset(PRUint32 aSpan)
{
  mBits &= ~COL_SPAN_OFFSET;
  mBits |= (aSpan << COL_SPAN_SHIFT);

  mBits |= SPAN;
  mBits |= COL_SPAN;
}

inline PRBool CellData::IsOverlap() const
{
  return (SPAN == (SPAN & mBits)) && (OVERLAP == (OVERLAP & mBits));
}

inline void CellData::SetOverlap(PRBool aOverlap)
{
  if (SPAN == (SPAN & mBits)) {
    if (aOverlap) {
      mBits |= OVERLAP;
    }
    else {
      mBits &= ~OVERLAP;
    }
  }
}

inline BCData::BCData()
{
  mLeftOwner = mTopOwner = eCellOwner;
  mLeftStart = mTopStart = 1;
  mLeftSize = mCornerSubSize = mTopSize = 0;
  mCornerSide = NS_SIDE_TOP;
  mCornerBevel = PR_FALSE;
}

inline BCData::~BCData()
{
}

inline nscoord BCData::GetLeftEdge(BCBorderOwner& aOwner,
                                   PRBool&        aStart) const
{
  aOwner = (BCBorderOwner)mLeftOwner;
  aStart = (PRBool)mLeftStart;

  return (nscoord)mLeftSize;
}

inline void BCData::SetLeftEdge(BCBorderOwner  aOwner,
                                nscoord        aSize,
                                PRBool         aStart)
{
  mLeftOwner = aOwner;
  mLeftSize  = (aSize > MAX_BORDER_WIDTH) ? MAX_BORDER_WIDTH : aSize;
  mLeftStart = aStart;
}

inline nscoord BCData::GetTopEdge(BCBorderOwner& aOwner,
                                  PRBool&        aStart) const
{
  aOwner = (BCBorderOwner)mTopOwner;
  aStart = (PRBool)mTopStart;

  return (nscoord)mTopSize;
}

inline void BCData::SetTopEdge(BCBorderOwner  aOwner,
                               nscoord        aSize,
                               PRBool         aStart)
{
  mTopOwner = aOwner;
  mTopSize  = (aSize > MAX_BORDER_WIDTH) ? MAX_BORDER_WIDTH : aSize;
  mTopStart = aStart;
}

inline BCPixelSize BCData::GetCorner(mozilla::css::Side& aOwnerSide,
                                     PRPackedBool&       aBevel) const
{
  aOwnerSide = mozilla::css::Side(mCornerSide);
  aBevel     = (PRBool)mCornerBevel;
  return mCornerSubSize;
}

inline void BCData::SetCorner(BCPixelSize aSubSize,
                              mozilla::css::Side aOwnerSide,
                              PRBool  aBevel)
{
  mCornerSubSize = aSubSize;
  mCornerSide    = aOwnerSide;
  mCornerBevel   = aBevel;
}

inline PRBool BCData::IsLeftStart() const
{
  return (PRBool)mLeftStart;
}

inline void BCData::SetLeftStart(PRBool aValue)
{
  mLeftStart = aValue;
}

inline PRBool BCData::IsTopStart() const
{
  return (PRBool)mTopStart;
}

inline void BCData::SetTopStart(PRBool aValue)
{
  mTopStart = aValue;
}

#endif
