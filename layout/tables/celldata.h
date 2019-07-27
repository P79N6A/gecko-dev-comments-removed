



#ifndef CellData_h__
#define CellData_h__

#include "nsISupports.h"
#include "nsCoord.h"
#include "mozilla/gfx/Types.h"
#include <stdint.h>

class nsTableCellFrame;
class nsCellMap;
class BCCellData;


#define MAX_ROWSPAN 65534 // the cellmap can not handle more.
#define MAX_COLSPAN 1000 // limit as IE and opera do.  If this ever changes,
                         




class CellData
{
public:
  


  void   Init(nsTableCellFrame* aCellFrame);

  


  bool IsOrig() const;

  




  bool IsDead() const;

  


  bool IsSpan() const;

  


  bool IsRowSpan() const;

  




  bool IsZeroRowSpan() const;

  


  void SetZeroRowSpan(bool aIsZero);

  


  uint32_t GetRowSpanOffset() const;

  


  void SetRowSpanOffset(uint32_t aSpan);

  


  bool IsColSpan() const;

  





  bool IsZeroColSpan() const;

  


  void SetZeroColSpan(bool aIsZero);

  


  uint32_t GetColSpanOffset() const;

  


  void SetColSpanOffset(uint32_t aSpan);

  


  bool IsOverlap() const;

  


  void SetOverlap(bool aOverlap);

  



  nsTableCellFrame* GetCellFrame() const;

private:
  friend class nsCellMap;
  friend class BCCellData;

  


  explicit CellData(nsTableCellFrame* aOrigCell);  

  
  ~CellData(); 

protected:

  
  
  
  
  
  union {
    nsTableCellFrame* mOrigCell;
    uintptr_t         mBits;
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

typedef uint16_t BCPixelSize;



#define MAX_BORDER_WIDTH nscoord((1u << (sizeof(BCPixelSize) * 8)) - 1)

static inline nscoord
BC_BORDER_TOP_HALF_COORD(int32_t p2t, uint16_t px)    { return (px - px / 2) * p2t; }
static inline nscoord
BC_BORDER_RIGHT_HALF_COORD(int32_t p2t, uint16_t px)  { return (     px / 2) * p2t; }
static inline nscoord
BC_BORDER_BOTTOM_HALF_COORD(int32_t p2t, uint16_t px) { return (     px / 2) * p2t; }
static inline nscoord
BC_BORDER_LEFT_HALF_COORD(int32_t p2t, uint16_t px)   { return (px - px / 2) * p2t; }

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
                      bool&        aStart) const;

  void SetLeftEdge(BCBorderOwner aOwner,
                   nscoord       aSize,
                   bool          aStart);

  nscoord GetTopEdge(BCBorderOwner& aOwner,
                     bool&        aStart) const;

  void SetTopEdge(BCBorderOwner aOwner,
                  nscoord       aSize,
                  bool          aStart);

  BCPixelSize GetCorner(mozilla::Side&       aCornerOwner,
                        bool&  aBevel) const;

  void SetCorner(BCPixelSize aSubSize,
                 mozilla::Side aOwner,
                 bool    aBevel);

  bool IsLeftStart() const;

  void SetLeftStart(bool aValue);

  bool IsTopStart() const;

  void SetTopStart(bool aValue);


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
  explicit BCCellData(nsTableCellFrame* aOrigCell);
  ~BCCellData();

  BCData mData;
};








#define COL_SPAN_SHIFT   22

#define ROW_SPAN_SHIFT   3


#define COL_SPAN_OFFSET  (0x3FF << COL_SPAN_SHIFT)

#define ROW_SPAN_OFFSET  (0xFFFF << ROW_SPAN_SHIFT)


#define SPAN             0x00000001 // there a row or col span
#define ROW_SPAN         0x00000002 // there is a row span
#define ROW_SPAN_0       0x00000004 // the row span is 0
#define COL_SPAN         (1 << (COL_SPAN_SHIFT - 3)) // there is a col span
#define COL_SPAN_0       (1 << (COL_SPAN_SHIFT - 2)) // the col span is 0
#define OVERLAP          (1 << (COL_SPAN_SHIFT - 1)) // there is a row span and
                                                     
                                                     

inline nsTableCellFrame* CellData::GetCellFrame() const
{
  if (SPAN != (SPAN & mBits)) {
    return mOrigCell;
  }
  return nullptr;
}

inline void CellData::Init(nsTableCellFrame* aCellFrame)
{
  mOrigCell = aCellFrame;
}

inline bool CellData::IsOrig() const
{
  return ((nullptr != mOrigCell) && (SPAN != (SPAN & mBits)));
}

inline bool CellData::IsDead() const
{
  return (0 == mBits);
}

inline bool CellData::IsSpan() const
{
  return (SPAN == (SPAN & mBits));
}

inline bool CellData::IsRowSpan() const
{
  return (SPAN     == (SPAN & mBits)) &&
         (ROW_SPAN == (ROW_SPAN & mBits));
}

inline bool CellData::IsZeroRowSpan() const
{
  return (SPAN       == (SPAN & mBits))     &&
         (ROW_SPAN   == (ROW_SPAN & mBits)) &&
         (ROW_SPAN_0 == (ROW_SPAN_0 & mBits));
}

inline void CellData::SetZeroRowSpan(bool aIsZeroSpan)
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

inline uint32_t CellData::GetRowSpanOffset() const
{
  if ((SPAN == (SPAN & mBits)) && ((ROW_SPAN == (ROW_SPAN & mBits)))) {
    return (uint32_t)((mBits & ROW_SPAN_OFFSET) >> ROW_SPAN_SHIFT);
  }
  return 0;
}

inline void CellData::SetRowSpanOffset(uint32_t aSpan)
{
  mBits &= ~ROW_SPAN_OFFSET;
  mBits |= (aSpan << ROW_SPAN_SHIFT);
  mBits |= SPAN;
  mBits |= ROW_SPAN;
}

inline bool CellData::IsColSpan() const
{
  return (SPAN     == (SPAN & mBits)) &&
         (COL_SPAN == (COL_SPAN & mBits));
}

inline bool CellData::IsZeroColSpan() const
{
  return (SPAN       == (SPAN & mBits))     &&
         (COL_SPAN   == (COL_SPAN & mBits)) &&
         (COL_SPAN_0 == (COL_SPAN_0 & mBits));
}

inline void CellData::SetZeroColSpan(bool aIsZeroSpan)
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

inline uint32_t CellData::GetColSpanOffset() const
{
  if ((SPAN == (SPAN & mBits)) && ((COL_SPAN == (COL_SPAN & mBits)))) {
    return (uint32_t)((mBits & COL_SPAN_OFFSET) >> COL_SPAN_SHIFT);
  }
  return 0;
}

inline void CellData::SetColSpanOffset(uint32_t aSpan)
{
  mBits &= ~COL_SPAN_OFFSET;
  mBits |= (aSpan << COL_SPAN_SHIFT);

  mBits |= SPAN;
  mBits |= COL_SPAN;
}

inline bool CellData::IsOverlap() const
{
  return (SPAN == (SPAN & mBits)) && (OVERLAP == (OVERLAP & mBits));
}

inline void CellData::SetOverlap(bool aOverlap)
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
  mCornerBevel = false;
}

inline BCData::~BCData()
{
}

inline nscoord BCData::GetLeftEdge(BCBorderOwner& aOwner,
                                   bool&        aStart) const
{
  aOwner = (BCBorderOwner)mLeftOwner;
  aStart = (bool)mLeftStart;

  return (nscoord)mLeftSize;
}

inline void BCData::SetLeftEdge(BCBorderOwner  aOwner,
                                nscoord        aSize,
                                bool           aStart)
{
  mLeftOwner = aOwner;
  mLeftSize  = (aSize > MAX_BORDER_WIDTH) ? MAX_BORDER_WIDTH : aSize;
  mLeftStart = aStart;
}

inline nscoord BCData::GetTopEdge(BCBorderOwner& aOwner,
                                  bool&        aStart) const
{
  aOwner = (BCBorderOwner)mTopOwner;
  aStart = (bool)mTopStart;

  return (nscoord)mTopSize;
}

inline void BCData::SetTopEdge(BCBorderOwner  aOwner,
                               nscoord        aSize,
                               bool           aStart)
{
  mTopOwner = aOwner;
  mTopSize  = (aSize > MAX_BORDER_WIDTH) ? MAX_BORDER_WIDTH : aSize;
  mTopStart = aStart;
}

inline BCPixelSize BCData::GetCorner(mozilla::Side& aOwnerSide,
                                     bool&       aBevel) const
{
  aOwnerSide = mozilla::Side(mCornerSide);
  aBevel     = (bool)mCornerBevel;
  return mCornerSubSize;
}

inline void BCData::SetCorner(BCPixelSize aSubSize,
                              mozilla::Side aOwnerSide,
                              bool    aBevel)
{
  mCornerSubSize = aSubSize;
  mCornerSide    = aOwnerSide;
  mCornerBevel   = aBevel;
}

inline bool BCData::IsLeftStart() const
{
  return (bool)mLeftStart;
}

inline void BCData::SetLeftStart(bool aValue)
{
  mLeftStart = aValue;
}

inline bool BCData::IsTopStart() const
{
  return (bool)mTopStart;
}

inline void BCData::SetTopStart(bool aValue)
{
  mTopStart = aValue;
}

#endif
