




#ifndef WritingModes_h_
#define WritingModes_h_

#include "nsRect.h"
#include "nsStyleContext.h"
#include "nsBidiUtils.h"
















#define CHECK_WRITING_MODE(param) \
   NS_ASSERTION(param == GetWritingMode(), "writing-mode mismatch")

namespace mozilla {


enum PhysicalAxis {
  eAxisVertical      = 0x0,
  eAxisHorizontal    = 0x1
};


enum LogicalAxis {
  eLogicalAxisBlock  = 0x0,
  eLogicalAxisInline = 0x1
};
enum LogicalEdge {
  eLogicalEdgeStart  = 0x0,
  eLogicalEdgeEnd    = 0x1
};
enum LogicalSide {
  eLogicalSideBStart = (eLogicalAxisBlock  << 1) | eLogicalEdgeStart,  
  eLogicalSideBEnd   = (eLogicalAxisBlock  << 1) | eLogicalEdgeEnd,    
  eLogicalSideIStart = (eLogicalAxisInline << 1) | eLogicalEdgeStart,  
  eLogicalSideIEnd   = (eLogicalAxisInline << 1) | eLogicalEdgeEnd     
};

inline bool IsInline(LogicalSide aSide) { return aSide & 0x2; }
inline bool IsBlock(LogicalSide aSide) { return !IsInline(aSide); }
inline bool IsEnd(LogicalSide aSide) { return aSide & 0x1; }
inline bool IsStart(LogicalSide aSide) { return !IsEnd(aSide); }

inline LogicalAxis GetAxis(LogicalSide aSide)
{
  return IsInline(aSide) ? eLogicalAxisInline : eLogicalAxisBlock;
}

inline LogicalEdge GetEdge(LogicalSide aSide)
{
  return IsEnd(aSide) ? eLogicalEdgeEnd : eLogicalEdgeStart;
}

inline LogicalEdge GetOppositeEdge(LogicalEdge aEdge)
{
  
  return LogicalEdge(1 - aEdge);
}

inline LogicalSide
MakeLogicalSide(LogicalAxis aAxis, LogicalEdge aEdge)
{
  return LogicalSide((aAxis << 1) | aEdge);
}

inline LogicalSide GetOppositeSide(LogicalSide aSide)
{
  return MakeLogicalSide(GetAxis(aSide), GetOppositeEdge(GetEdge(aSide)));
}

enum LogicalSideBits {
  eLogicalSideBitsNone   = 0,
  eLogicalSideBitsBStart = 1 << eLogicalSideBStart,
  eLogicalSideBitsBEnd   = 1 << eLogicalSideBEnd,
  eLogicalSideBitsIEnd   = 1 << eLogicalSideIEnd,
  eLogicalSideBitsIStart = 1 << eLogicalSideIStart,
  eLogicalSideBitsBBoth = eLogicalSideBitsBStart | eLogicalSideBitsBEnd,
  eLogicalSideBitsIBoth = eLogicalSideBitsIStart | eLogicalSideBitsIEnd,
  eLogicalSideBitsAll = eLogicalSideBitsBBoth | eLogicalSideBitsIBoth
};

enum LineRelativeDir {
  eLineRelativeDirOver  = eLogicalSideBStart,
  eLineRelativeDirUnder = eLogicalSideBEnd,
  eLineRelativeDirLeft  = eLogicalSideIStart,
  eLineRelativeDirRight = eLogicalSideIEnd
};




struct LogicalSides final {
  LogicalSides() : mBits(0) {}
  explicit LogicalSides(LogicalSideBits aSideBits)
  {
    MOZ_ASSERT((aSideBits & ~eLogicalSideBitsAll) == 0, "illegal side bits");
    mBits = aSideBits;
  }
  bool IsEmpty() const { return mBits == 0; }
  bool BStart()  const { return mBits & eLogicalSideBitsBStart; }
  bool BEnd()    const { return mBits & eLogicalSideBitsBEnd; }
  bool IStart()  const { return mBits & eLogicalSideBitsIStart; }
  bool IEnd()    const { return mBits & eLogicalSideBitsIEnd; }
  bool Contains(LogicalSideBits aSideBits) const
  {
    MOZ_ASSERT((aSideBits & ~eLogicalSideBitsAll) == 0, "illegal side bits");
    return (mBits & aSideBits) == aSideBits;
  }
  LogicalSides operator|(LogicalSides aOther) const
  {
    return LogicalSides(LogicalSideBits(mBits | aOther.mBits));
  }
  LogicalSides operator|(LogicalSideBits aSideBits) const
  {
    return *this | LogicalSides(aSideBits);
  }
  LogicalSides& operator|=(LogicalSides aOther)
  {
    mBits |= aOther.mBits;
    return *this;
  }
  LogicalSides& operator|=(LogicalSideBits aSideBits)
  {
    return *this |= LogicalSides(aSideBits);
  }
  bool operator==(LogicalSides aOther) const
  {
    return mBits == aOther.mBits;
  }
  bool operator!=(LogicalSides aOther) const
  {
    return !(*this == aOther);
  }

private:
  uint8_t mBits;
};













class WritingMode {
public:
  


  enum InlineDir {
    eInlineLTR = 0x00, 
    eInlineRTL = 0x02, 
    eInlineTTB = 0x01, 
    eInlineBTT = 0x03, 
  };

  


  enum BlockDir {
    eBlockTB = 0x00, 
    eBlockRL = 0x01, 
    eBlockLR = 0x05, 
  };

  


  enum BidiDir {
    eBidiLTR = 0x00, 
    eBidiRTL = 0x10, 
  };

  


  enum {
    eUnknownWritingMode = 0xff
  };

  


  InlineDir GetInlineDir() const { return InlineDir(mWritingMode & eInlineMask); }

  


  BlockDir GetBlockDir() const { return BlockDir(mWritingMode & eBlockMask); }

  


  BidiDir GetBidiDir() const { return BidiDir(mWritingMode & eBidiMask); }

  


  bool IsBidiLTR() const { return eBidiLTR == GetBidiDir(); }

  


  bool IsVerticalLR() const { return eBlockLR == GetBlockDir(); }

  


  bool IsVerticalRL() const { return eBlockRL == GetBlockDir(); }

  



  bool IsVertical() const { return !!(mWritingMode & eOrientationMask); }

  





  bool IsLineInverted() const { return !!(mWritingMode & eLineOrientMask); }

  





  int FlowRelativeToLineRelativeFactor() const
  {
    return IsLineInverted() ? -1 : 1;
  }

  







  bool IsSideways() const { return !!(mWritingMode & eSidewaysMask); }

  static mozilla::PhysicalAxis PhysicalAxisForLogicalAxis(
                                              uint8_t aWritingModeValue,
                                              LogicalAxis aAxis)
  {
    
    
    
    
    static_assert(NS_STYLE_WRITING_MODE_HORIZONTAL_TB == 0 &&
                  NS_STYLE_WRITING_MODE_VERTICAL_RL == 1 &&
                  NS_STYLE_WRITING_MODE_VERTICAL_LR == 3 &&
                  eLogicalAxisBlock == 0 &&
                  eLogicalAxisInline == 1 &&
                  eAxisVertical == 0 && 
                  eAxisHorizontal == 1,
                  "unexpected writing-mode, logical axis or physical axis "
                  "constant values");
    return mozilla::PhysicalAxis((aWritingModeValue ^ aAxis) & 0x1);
  }

  mozilla::PhysicalAxis PhysicalAxis(LogicalAxis aAxis) const
  {
    
    
    
    
    
    int wm = mWritingMode & eOrientationMask;
    return PhysicalAxisForLogicalAxis(wm, aAxis);
  }

  static mozilla::Side PhysicalSideForBlockAxis(uint8_t aWritingModeValue,
                                                LogicalEdge aEdge)
  {
    
    
    
    
    static const mozilla::css::Side kLogicalBlockSides[][2] = {
      { NS_SIDE_TOP,    NS_SIDE_BOTTOM },  
      { NS_SIDE_RIGHT,  NS_SIDE_LEFT   },  
      { NS_SIDE_BOTTOM, NS_SIDE_TOP    },  
      { NS_SIDE_LEFT,   NS_SIDE_RIGHT  },  
    };

    NS_ASSERTION(aWritingModeValue < 4, "invalid aWritingModeValue value");
    return kLogicalBlockSides[aWritingModeValue][aEdge];
  }

  mozilla::Side PhysicalSideForInlineAxis(LogicalEdge aEdge) const
  {
    
    
    
    
    
    static const mozilla::css::Side kLogicalInlineSides[][2] = {
      { NS_SIDE_LEFT,   NS_SIDE_RIGHT  },  
      { NS_SIDE_TOP,    NS_SIDE_BOTTOM },  
      { NS_SIDE_RIGHT,  NS_SIDE_LEFT   },  
      { NS_SIDE_BOTTOM, NS_SIDE_TOP    },  
      { NS_SIDE_RIGHT,  NS_SIDE_LEFT   },  
      { NS_SIDE_TOP,    NS_SIDE_BOTTOM },  
      { NS_SIDE_LEFT,   NS_SIDE_RIGHT  },  
      { NS_SIDE_BOTTOM, NS_SIDE_TOP    },  
      { NS_SIDE_LEFT,   NS_SIDE_RIGHT  },  
      { NS_SIDE_TOP,    NS_SIDE_BOTTOM },  
      { NS_SIDE_RIGHT,  NS_SIDE_LEFT   },  
      { NS_SIDE_BOTTOM, NS_SIDE_TOP    },  
      { NS_SIDE_LEFT,   NS_SIDE_RIGHT  },  
      { NS_SIDE_TOP,    NS_SIDE_BOTTOM },  
      { NS_SIDE_RIGHT,  NS_SIDE_LEFT   },  
      { NS_SIDE_BOTTOM, NS_SIDE_TOP    },  
    };

    
    
    
    
    static_assert(eOrientationMask == 0x01 && eInlineFlowMask == 0x02 &&
                  eBlockFlowMask == 0x04 && eLineOrientMask == 0x08,
                  "unexpected mask values");
    int index = mWritingMode & 0x0F;
    return kLogicalInlineSides[index][aEdge];
  }

  



  mozilla::Side PhysicalSide(LogicalSide aSide) const
  {
    if (IsBlock(aSide)) {
      static_assert(eOrientationMask == 0x01 && eBlockFlowMask == 0x04,
                    "unexpected mask values");
      int wm = ((mWritingMode & eBlockFlowMask) >> 1) |
               (mWritingMode & eOrientationMask);
      return PhysicalSideForBlockAxis(wm, GetEdge(aSide));
    }

    return PhysicalSideForInlineAxis(GetEdge(aSide));
  }

  



  LogicalSide LogicalSideForLineRelativeDir(LineRelativeDir aDir) const
  {
    auto side = static_cast<LogicalSide>(aDir);
    if (IsInline(side)) {
      return IsBidiLTR() ? side : GetOppositeSide(side);
    }
    return !IsLineInverted() ? side : GetOppositeSide(side);
  }

  




  WritingMode()
    : mWritingMode(0)
  { }

  


  explicit WritingMode(nsStyleContext* aStyleContext)
  {
    NS_ASSERTION(aStyleContext, "we need an nsStyleContext here");

    const nsStyleVisibility* styleVisibility = aStyleContext->StyleVisibility();

    switch (styleVisibility->mWritingMode) {
      case NS_STYLE_WRITING_MODE_HORIZONTAL_TB:
        mWritingMode = 0;
        break;

      case NS_STYLE_WRITING_MODE_VERTICAL_LR:
      {
        mWritingMode = eBlockFlowMask |
                       eLineOrientMask |
                       eOrientationMask;
        uint8_t textOrientation = aStyleContext->StyleVisibility()->mTextOrientation;
#if 0 
        if (textOrientation == NS_STYLE_TEXT_ORIENTATION_SIDEWAYS_LEFT) {
          mWritingMode &= ~eLineOrientMask;
        }
#endif
        if (textOrientation >= NS_STYLE_TEXT_ORIENTATION_SIDEWAYS_RIGHT) {
          mWritingMode |= eSidewaysMask;
        }
        break;
      }

      case NS_STYLE_WRITING_MODE_VERTICAL_RL:
      {
        mWritingMode = eOrientationMask;
        uint8_t textOrientation = aStyleContext->StyleVisibility()->mTextOrientation;
#if 0 
        if (textOrientation == NS_STYLE_TEXT_ORIENTATION_SIDEWAYS_LEFT) {
          mWritingMode |= eLineOrientMask;
        }
#endif
        if (textOrientation >= NS_STYLE_TEXT_ORIENTATION_SIDEWAYS_RIGHT) {
          mWritingMode |= eSidewaysMask;
        }
        break;
      }

      default:
        NS_NOTREACHED("unknown writing mode!");
        mWritingMode = 0;
        break;
    }

    if (NS_STYLE_DIRECTION_RTL == styleVisibility->mDirection) {
      mWritingMode |= eInlineFlowMask | 
                      eBidiMask;
    }
  }

  
  

  
  void SetDirectionFromBidiLevel(uint8_t level)
  {
    if (IS_LEVEL_RTL(level)) {
      
      mWritingMode |= eBidiMask;
    } else {
      
      mWritingMode &= ~eBidiMask;
    }
  }

  


  bool operator==(const WritingMode& aOther) const
  {
    return mWritingMode == aOther.mWritingMode;
  }

  bool operator!=(const WritingMode& aOther) const
  {
    return mWritingMode != aOther.mWritingMode;
  }

  


  bool IsOrthogonalTo(const WritingMode& aOther) const
  {
    return IsVertical() != aOther.IsVertical();
  }

private:
  friend class LogicalPoint;
  friend class LogicalSize;
  friend class LogicalMargin;
  friend class LogicalRect;

  friend struct IPC::ParamTraits<WritingMode>;

  


  static inline WritingMode Unknown()
  {
    return WritingMode(eUnknownWritingMode);
  }

  



  explicit WritingMode(uint8_t aValue)
    : mWritingMode(aValue)
  { }

  uint8_t mWritingMode;

  enum Masks {
    
    eOrientationMask = 0x01, 
    eInlineFlowMask  = 0x02, 
    eBlockFlowMask   = 0x04, 
    eLineOrientMask  = 0x08, 
    eBidiMask        = 0x10, 
    
    

    eSidewaysMask    = 0x20, 
                             
                             

    
    eInlineMask = 0x03,
    eBlockMask  = 0x05
  };
};









































class LogicalPoint {
public:
  explicit LogicalPoint(WritingMode aWritingMode)
    :
#ifdef DEBUG
      mWritingMode(aWritingMode),
#endif
      mPoint(0, 0)
  { }

  
  
  LogicalPoint(WritingMode aWritingMode, nscoord aI, nscoord aB)
    :
#ifdef DEBUG
      mWritingMode(aWritingMode),
#endif
      mPoint(aI, aB)
  { }

  
  
  
  LogicalPoint(WritingMode aWritingMode,
               const nsPoint& aPoint,
               nscoord aContainerWidth)
#ifdef DEBUG
    : mWritingMode(aWritingMode)
#endif
  {
    if (aWritingMode.IsVertical()) {
      I() = aPoint.y;
      B() = aWritingMode.IsVerticalLR() ? aPoint.x : aContainerWidth - aPoint.x;
    } else {
      I() = aWritingMode.IsBidiLTR() ? aPoint.x : aContainerWidth - aPoint.x;
      B() = aPoint.y;
    }
  }

  



  nscoord I(WritingMode aWritingMode) const 
  {
    CHECK_WRITING_MODE(aWritingMode);
    return mPoint.x;
  }
  nscoord B(WritingMode aWritingMode) const 
  {
    CHECK_WRITING_MODE(aWritingMode);
    return mPoint.y;
  }

  nscoord X(WritingMode aWritingMode, nscoord aContainerWidth) const
  {
    CHECK_WRITING_MODE(aWritingMode);
    if (aWritingMode.IsVertical()) {
      return aWritingMode.IsVerticalLR() ? B() : aContainerWidth - B();
    } else {
      return aWritingMode.IsBidiLTR() ? I() : aContainerWidth - I();
    }
  }
  nscoord Y(WritingMode aWritingMode) const
  {
    CHECK_WRITING_MODE(aWritingMode);
    return aWritingMode.IsVertical() ? I() : B();
  }

  



  nscoord& I(WritingMode aWritingMode) 
  {
    CHECK_WRITING_MODE(aWritingMode);
    return mPoint.x;
  }
  nscoord& B(WritingMode aWritingMode) 
  {
    CHECK_WRITING_MODE(aWritingMode);
    return mPoint.y;
  }

  


  void SetX(WritingMode aWritingMode, nscoord aX, nscoord aContainerWidth)
  {
    CHECK_WRITING_MODE(aWritingMode);
    if (aWritingMode.IsVertical()) {
      B() = aWritingMode.IsVerticalLR() ? aX : aContainerWidth - aX;
    } else {
      I() = aWritingMode.IsBidiLTR() ? aX : aContainerWidth - aX;
    }
  }
  void SetY(WritingMode aWritingMode, nscoord aY)
  {
    CHECK_WRITING_MODE(aWritingMode);
    if (aWritingMode.IsVertical()) {
      B() = aY;
    } else {
      I() = aY;
    }
  }

  



  nsPoint GetPhysicalPoint(WritingMode aWritingMode,
                           nscoord aContainerWidth) const
  {
    CHECK_WRITING_MODE(aWritingMode);
    if (aWritingMode.IsVertical()) {
      return nsPoint(aWritingMode.IsVerticalLR() ? B() : aContainerWidth - B(),
                     I());
    } else {
      return nsPoint(aWritingMode.IsBidiLTR() ? I() : aContainerWidth - I(),
                     B());
    }
  }

  


  LogicalPoint ConvertTo(WritingMode aToMode, WritingMode aFromMode,
                         nscoord aContainerWidth) const
  {
    CHECK_WRITING_MODE(aFromMode);
    return aToMode == aFromMode ?
      *this : LogicalPoint(aToMode,
                           GetPhysicalPoint(aFromMode, aContainerWidth),
                           aContainerWidth);
  }

  bool operator==(const LogicalPoint& aOther) const
  {
    CHECK_WRITING_MODE(aOther.GetWritingMode());
    return mPoint == aOther.mPoint;
  }

  bool operator!=(const LogicalPoint& aOther) const
  {
    CHECK_WRITING_MODE(aOther.GetWritingMode());
    return mPoint != aOther.mPoint;
  }

  LogicalPoint operator+(const LogicalPoint& aOther) const
  {
    CHECK_WRITING_MODE(aOther.GetWritingMode());
    
    
    
    return LogicalPoint(GetWritingMode(),
                        mPoint.x + aOther.mPoint.x,
                        mPoint.y + aOther.mPoint.y);
  }

  LogicalPoint& operator+=(const LogicalPoint& aOther)
  {
    CHECK_WRITING_MODE(aOther.GetWritingMode());
    I() += aOther.I();
    B() += aOther.B();
    return *this;
  }

  LogicalPoint operator-(const LogicalPoint& aOther) const
  {
    CHECK_WRITING_MODE(aOther.GetWritingMode());
    
    
    
    return LogicalPoint(GetWritingMode(),
                        mPoint.x - aOther.mPoint.x,
                        mPoint.y - aOther.mPoint.y);
  }

  LogicalPoint& operator-=(const LogicalPoint& aOther)
  {
    CHECK_WRITING_MODE(aOther.GetWritingMode());
    I() -= aOther.I();
    B() -= aOther.B();
    return *this;
  }

private:
  friend class LogicalRect;

  











#ifdef DEBUG
  WritingMode GetWritingMode() const { return mWritingMode; }
#else
  WritingMode GetWritingMode() const { return WritingMode::Unknown(); }
#endif

  
  LogicalPoint() = delete;

  
  
  
  nscoord I() const 
  {
    return mPoint.x;
  }
  nscoord B() const 
  {
    return mPoint.y;
  }

  nscoord& I() 
  {
    return mPoint.x;
  }
  nscoord& B() 
  {
    return mPoint.y;
  }

#ifdef DEBUG
  WritingMode mWritingMode;
#endif

  
  
  
  
  nsPoint mPoint;
};




class LogicalSize {
public:
  explicit LogicalSize(WritingMode aWritingMode)
    :
#ifdef DEBUG
      mWritingMode(aWritingMode),
#endif
      mSize(0, 0)
  { }

  LogicalSize(WritingMode aWritingMode, nscoord aISize, nscoord aBSize)
    :
#ifdef DEBUG
      mWritingMode(aWritingMode),
#endif
      mSize(aISize, aBSize)
  { }

  LogicalSize(WritingMode aWritingMode, const nsSize& aPhysicalSize)
#ifdef DEBUG
    : mWritingMode(aWritingMode)
#endif
  {
    if (aWritingMode.IsVertical()) {
      ISize() = aPhysicalSize.height;
      BSize() = aPhysicalSize.width;
    } else {
      ISize() = aPhysicalSize.width;
      BSize() = aPhysicalSize.height;
    }
  }

  void SizeTo(WritingMode aWritingMode, nscoord aISize, nscoord aBSize)
  {
    CHECK_WRITING_MODE(aWritingMode);
    mSize.SizeTo(aISize, aBSize);
  }

  


  nscoord ISize(WritingMode aWritingMode) const 
  {
    CHECK_WRITING_MODE(aWritingMode);
    return mSize.width;
  }
  nscoord BSize(WritingMode aWritingMode) const 
  {
    CHECK_WRITING_MODE(aWritingMode);
    return mSize.height;
  }

  nscoord Width(WritingMode aWritingMode) const
  {
    CHECK_WRITING_MODE(aWritingMode);
    return aWritingMode.IsVertical() ? BSize() : ISize();
  }
  nscoord Height(WritingMode aWritingMode) const
  {
    CHECK_WRITING_MODE(aWritingMode);
    return aWritingMode.IsVertical() ? ISize() : BSize();
  }

  


  nscoord& ISize(WritingMode aWritingMode) 
  {
    CHECK_WRITING_MODE(aWritingMode);
    return mSize.width;
  }
  nscoord& BSize(WritingMode aWritingMode) 
  {
    CHECK_WRITING_MODE(aWritingMode);
    return mSize.height;
  }

  nscoord& Width(WritingMode aWritingMode)
  {
    CHECK_WRITING_MODE(aWritingMode);
    return aWritingMode.IsVertical() ? BSize() : ISize();
  }
  nscoord& Height(WritingMode aWritingMode)
  {
    CHECK_WRITING_MODE(aWritingMode);
    return aWritingMode.IsVertical() ? ISize() : BSize();
  }

  


  nsSize GetPhysicalSize(WritingMode aWritingMode) const
  {
    CHECK_WRITING_MODE(aWritingMode);
    return aWritingMode.IsVertical() ?
      nsSize(BSize(), ISize()) : nsSize(ISize(), BSize());
  }

  


  LogicalSize ConvertTo(WritingMode aToMode, WritingMode aFromMode) const
  {
#ifdef DEBUG
    
    
    CHECK_WRITING_MODE(aFromMode);
    return aToMode == aFromMode ?
      *this : LogicalSize(aToMode, GetPhysicalSize(aFromMode));
#else
    
    
    return (aToMode == aFromMode || !aToMode.IsOrthogonalTo(aFromMode))
             ? *this : LogicalSize(aToMode, BSize(), ISize());
#endif
  }

  


  bool IsAllZero() const
  {
    return ISize() == 0 && BSize() == 0;
  }

  



  bool operator==(const LogicalSize& aOther) const
  {
    CHECK_WRITING_MODE(aOther.GetWritingMode());
    return mSize == aOther.mSize;
  }

  bool operator!=(const LogicalSize& aOther) const
  {
    CHECK_WRITING_MODE(aOther.GetWritingMode());
    return mSize != aOther.mSize;
  }

  LogicalSize operator+(const LogicalSize& aOther) const
  {
    CHECK_WRITING_MODE(aOther.GetWritingMode());
    return LogicalSize(GetWritingMode(), ISize() + aOther.ISize(),
                                         BSize() + aOther.BSize());
  }
  LogicalSize& operator+=(const LogicalSize& aOther)
  {
    CHECK_WRITING_MODE(aOther.GetWritingMode());
    ISize() += aOther.ISize();
    BSize() += aOther.BSize();
    return *this;
  }

  LogicalSize operator-(const LogicalSize& aOther) const
  {
    CHECK_WRITING_MODE(aOther.GetWritingMode());
    return LogicalSize(GetWritingMode(), ISize() - aOther.ISize(),
                                         BSize() - aOther.BSize());
  }
  LogicalSize& operator-=(const LogicalSize& aOther)
  {
    CHECK_WRITING_MODE(aOther.GetWritingMode());
    ISize() -= aOther.ISize();
    BSize() -= aOther.BSize();
    return *this;
  }

private:
  friend class LogicalRect;

  LogicalSize() = delete;

#ifdef DEBUG
  WritingMode GetWritingMode() const { return mWritingMode; }
#else
  WritingMode GetWritingMode() const { return WritingMode::Unknown(); }
#endif

  nscoord ISize() const 
  {
    return mSize.width;
  }
  nscoord BSize() const 
  {
    return mSize.height;
  }

  nscoord& ISize() 
  {
    return mSize.width;
  }
  nscoord& BSize() 
  {
    return mSize.height;
  }

#ifdef DEBUG
  WritingMode mWritingMode;
#endif
  nsSize      mSize;
};




class LogicalMargin {
public:
  explicit LogicalMargin(WritingMode aWritingMode)
    :
#ifdef DEBUG
      mWritingMode(aWritingMode),
#endif
      mMargin(0, 0, 0, 0)
  { }

  LogicalMargin(WritingMode aWritingMode,
                nscoord aBStart, nscoord aIEnd,
                nscoord aBEnd, nscoord aIStart)
    :
#ifdef DEBUG
      mWritingMode(aWritingMode),
#endif
      mMargin(aBStart, aIEnd, aBEnd, aIStart)
  { }

  LogicalMargin(WritingMode aWritingMode, const nsMargin& aPhysicalMargin)
#ifdef DEBUG
    : mWritingMode(aWritingMode)
#endif
  {
    if (aWritingMode.IsVertical()) {
      if (aWritingMode.IsVerticalLR()) {
        mMargin.top = aPhysicalMargin.left;
        mMargin.bottom = aPhysicalMargin.right;
      } else {
        mMargin.top = aPhysicalMargin.right;
        mMargin.bottom = aPhysicalMargin.left;
      }
      if (aWritingMode.IsBidiLTR()) {
        mMargin.left = aPhysicalMargin.top;
        mMargin.right = aPhysicalMargin.bottom;
      } else {
        mMargin.left = aPhysicalMargin.bottom;
        mMargin.right = aPhysicalMargin.top;
      }
    } else {
      mMargin.top = aPhysicalMargin.top;
      mMargin.bottom = aPhysicalMargin.bottom;
      if (aWritingMode.IsBidiLTR()) {
        mMargin.left = aPhysicalMargin.left;
        mMargin.right = aPhysicalMargin.right;
      } else {
        mMargin.left = aPhysicalMargin.right;
        mMargin.right = aPhysicalMargin.left;
      }
    }
  }

  nscoord IStart(WritingMode aWritingMode) const 
  {
    CHECK_WRITING_MODE(aWritingMode);
    return mMargin.left;
  }
  nscoord IEnd(WritingMode aWritingMode) const 
  {
    CHECK_WRITING_MODE(aWritingMode);
    return mMargin.right;
  }
  nscoord BStart(WritingMode aWritingMode) const 
  {
    CHECK_WRITING_MODE(aWritingMode);
    return mMargin.top;
  }
  nscoord BEnd(WritingMode aWritingMode) const 
  {
    CHECK_WRITING_MODE(aWritingMode);
    return mMargin.bottom;
  }

  nscoord& IStart(WritingMode aWritingMode) 
  {
    CHECK_WRITING_MODE(aWritingMode);
    return mMargin.left;
  }
  nscoord& IEnd(WritingMode aWritingMode) 
  {
    CHECK_WRITING_MODE(aWritingMode);
    return mMargin.right;
  }
  nscoord& BStart(WritingMode aWritingMode) 
  {
    CHECK_WRITING_MODE(aWritingMode);
    return mMargin.top;
  }
  nscoord& BEnd(WritingMode aWritingMode) 
  {
    CHECK_WRITING_MODE(aWritingMode);
    return mMargin.bottom;
  }

  nscoord IStartEnd(WritingMode aWritingMode) const 
  {
    CHECK_WRITING_MODE(aWritingMode);
    return mMargin.LeftRight();
  }
  nscoord BStartEnd(WritingMode aWritingMode) const 
  {
    CHECK_WRITING_MODE(aWritingMode);
    return mMargin.TopBottom();
  }

  



  LogicalSize Size(WritingMode aWritingMode) const
  {
    CHECK_WRITING_MODE(aWritingMode);
    return LogicalSize(aWritingMode, IStartEnd(), BStartEnd());
  }

  



  nscoord Top(WritingMode aWritingMode) const
  {
    CHECK_WRITING_MODE(aWritingMode);
    return aWritingMode.IsVertical() ?
      (aWritingMode.IsBidiLTR() ? IStart() : IEnd()) : BStart();
  }

  nscoord Bottom(WritingMode aWritingMode) const
  {
    CHECK_WRITING_MODE(aWritingMode);
    return aWritingMode.IsVertical() ?
      (aWritingMode.IsBidiLTR() ? IEnd() : IStart()) : BEnd();
  }

  nscoord Left(WritingMode aWritingMode) const
  {
    CHECK_WRITING_MODE(aWritingMode);
    return aWritingMode.IsVertical() ?
      (aWritingMode.IsVerticalLR() ? BStart() : BEnd()) :
      (aWritingMode.IsBidiLTR() ? IStart() : IEnd());
  }

  nscoord Right(WritingMode aWritingMode) const
  {
    CHECK_WRITING_MODE(aWritingMode);
    return aWritingMode.IsVertical() ?
      (aWritingMode.IsVerticalLR() ? BEnd() : BStart()) :
      (aWritingMode.IsBidiLTR() ? IEnd() : IStart());
  }

  nscoord LeftRight(WritingMode aWritingMode) const
  {
    CHECK_WRITING_MODE(aWritingMode);
    return aWritingMode.IsVertical() ? BStartEnd() : IStartEnd();
  }

  nscoord TopBottom(WritingMode aWritingMode) const
  {
    CHECK_WRITING_MODE(aWritingMode);
    return aWritingMode.IsVertical() ? IStartEnd() : BStartEnd();
  }

  void SizeTo(WritingMode aWritingMode,
              nscoord aBStart, nscoord aIEnd, nscoord aBEnd, nscoord aIStart)
  {
    CHECK_WRITING_MODE(aWritingMode);
    mMargin.SizeTo(aBStart, aIEnd, aBEnd, aIStart);
  }

  nscoord& Top(WritingMode aWritingMode)
  {
    CHECK_WRITING_MODE(aWritingMode);
    return aWritingMode.IsVertical() ?
      (aWritingMode.IsBidiLTR() ? IStart() : IEnd()) : BStart();
  }

  nscoord& Bottom(WritingMode aWritingMode)
  {
    CHECK_WRITING_MODE(aWritingMode);
    return aWritingMode.IsVertical() ?
      (aWritingMode.IsBidiLTR() ? IEnd() : IStart()) : BEnd();
  }

  nscoord& Left(WritingMode aWritingMode)
  {
    CHECK_WRITING_MODE(aWritingMode);
    return aWritingMode.IsVertical() ?
      (aWritingMode.IsVerticalLR() ? BStart() : BEnd()) :
      (aWritingMode.IsBidiLTR() ? IStart() : IEnd());
  }

  nscoord& Right(WritingMode aWritingMode)
  {
    CHECK_WRITING_MODE(aWritingMode);
    return aWritingMode.IsVertical() ?
      (aWritingMode.IsVerticalLR() ? BEnd() : BStart()) :
      (aWritingMode.IsBidiLTR() ? IEnd() : IStart());
  }

  


  nsMargin GetPhysicalMargin(WritingMode aWritingMode) const
  {
    CHECK_WRITING_MODE(aWritingMode);
    return aWritingMode.IsVertical() ?
      (aWritingMode.IsVerticalLR() ?
        nsMargin(IStart(), BEnd(), IEnd(), BStart()) :
        nsMargin(IStart(), BStart(), IEnd(), BEnd())) :
      (aWritingMode.IsBidiLTR() ?
        nsMargin(BStart(), IEnd(), BEnd(), IStart()) :
        nsMargin(BStart(), IStart(), BEnd(), IEnd()));
  }

  



  LogicalMargin ConvertTo(WritingMode aToMode, WritingMode aFromMode) const
  {
    CHECK_WRITING_MODE(aFromMode);
    return aToMode == aFromMode ?
      *this : LogicalMargin(aToMode, GetPhysicalMargin(aFromMode));
  }

  void ApplySkipSides(LogicalSides aSkipSides)
  {
    if (aSkipSides.BStart()) {
      BStart() = 0;
    }
    if (aSkipSides.BEnd()) {
      BEnd() = 0;
    }
    if (aSkipSides.IStart()) {
      IStart() = 0;
    }
    if (aSkipSides.IEnd()) {
      IEnd() = 0;
    }
  }

  bool IsAllZero() const
  {
    return (mMargin.left == 0 && mMargin.top == 0 &&
            mMargin.right == 0 && mMargin.bottom == 0);
  }

  LogicalMargin operator+(const LogicalMargin& aMargin) const {
    CHECK_WRITING_MODE(aMargin.GetWritingMode());
    return LogicalMargin(GetWritingMode(),
                         BStart() + aMargin.BStart(),
                         IEnd() + aMargin.IEnd(),
                         BEnd() + aMargin.BEnd(),
                         IStart() + aMargin.IStart());
  }

  LogicalMargin operator-(const LogicalMargin& aMargin) const {
    CHECK_WRITING_MODE(aMargin.GetWritingMode());
    return LogicalMargin(GetWritingMode(),
                         BStart() - aMargin.BStart(),
                         IEnd() - aMargin.IEnd(),
                         BEnd() - aMargin.BEnd(),
                         IStart() - aMargin.IStart());
  }

private:
  friend class LogicalRect;

  LogicalMargin() = delete;

#ifdef DEBUG
  WritingMode GetWritingMode() const { return mWritingMode; }
#else
  WritingMode GetWritingMode() const { return WritingMode::Unknown(); }
#endif

  nscoord IStart() const 
  {
    return mMargin.left;
  }
  nscoord IEnd() const 
  {
    return mMargin.right;
  }
  nscoord BStart() const 
  {
    return mMargin.top;
  }
  nscoord BEnd() const 
  {
    return mMargin.bottom;
  }

  nscoord& IStart() 
  {
    return mMargin.left;
  }
  nscoord& IEnd() 
  {
    return mMargin.right;
  }
  nscoord& BStart() 
  {
    return mMargin.top;
  }
  nscoord& BEnd() 
  {
    return mMargin.bottom;
  }

  nscoord IStartEnd() const 
  {
    return mMargin.LeftRight();
  }
  nscoord BStartEnd() const 
  {
    return mMargin.TopBottom();
  }

#ifdef DEBUG
  WritingMode mWritingMode;
#endif
  nsMargin    mMargin;
};




class LogicalRect {
public:
  explicit LogicalRect(WritingMode aWritingMode)
    :
#ifdef DEBUG
      mWritingMode(aWritingMode),
#endif
      mRect(0, 0, 0, 0)
  { }

  LogicalRect(WritingMode aWritingMode,
              nscoord aIStart, nscoord aBStart,
              nscoord aISize, nscoord aBSize)
    :
#ifdef DEBUG
      mWritingMode(aWritingMode),
#endif
      mRect(aIStart, aBStart, aISize, aBSize)
  { }

  LogicalRect(WritingMode aWritingMode,
              const LogicalPoint& aOrigin,
              const LogicalSize& aSize)
    : 
#ifdef DEBUG
      mWritingMode(aWritingMode),
#endif
      mRect(aOrigin.mPoint, aSize.mSize)
  {
    CHECK_WRITING_MODE(aOrigin.GetWritingMode());
    CHECK_WRITING_MODE(aSize.GetWritingMode());
  }

  LogicalRect(WritingMode aWritingMode,
              const nsRect& aRect,
              nscoord aContainerWidth)
#ifdef DEBUG
    : mWritingMode(aWritingMode)
#endif
  {
    if (aWritingMode.IsVertical()) {
      if (aWritingMode.IsVerticalLR()) {
        mRect.y = aRect.x;
      } else {
        mRect.y = aContainerWidth - aRect.XMost();
      }
      mRect.height = aRect.width;
      mRect.x = aRect.y;
      mRect.width = aRect.height;
    } else {
      if (aWritingMode.IsBidiLTR()) {
        mRect.x = aRect.x;
      } else {
        mRect.x = aContainerWidth - aRect.XMost();
      }
      mRect.width = aRect.width;
      mRect.y = aRect.y;
      mRect.height = aRect.height;
    }
  }

  


  nscoord IStart(WritingMode aWritingMode) const 
  {
    CHECK_WRITING_MODE(aWritingMode);
    return mRect.X();
  }
  nscoord IEnd(WritingMode aWritingMode) const 
  {
    CHECK_WRITING_MODE(aWritingMode);
    return mRect.XMost();
  }
  nscoord ISize(WritingMode aWritingMode) const 
  {
    CHECK_WRITING_MODE(aWritingMode);
    return mRect.Width();
  }

  nscoord BStart(WritingMode aWritingMode) const 
  {
    CHECK_WRITING_MODE(aWritingMode);
    return mRect.Y();
  }
  nscoord BEnd(WritingMode aWritingMode) const 
  {
    CHECK_WRITING_MODE(aWritingMode);
    return mRect.YMost();
  }
  nscoord BSize(WritingMode aWritingMode) const 
  {
    CHECK_WRITING_MODE(aWritingMode);
    return mRect.Height();
  }

  



  nscoord& IStart(WritingMode aWritingMode) 
  {
    CHECK_WRITING_MODE(aWritingMode);
    return mRect.x;
  }
  nscoord& ISize(WritingMode aWritingMode) 
  {
    CHECK_WRITING_MODE(aWritingMode);
    return mRect.width;
  }
  nscoord& BStart(WritingMode aWritingMode) 
  {
    CHECK_WRITING_MODE(aWritingMode);
    return mRect.y;
  }
  nscoord& BSize(WritingMode aWritingMode) 
  {
    CHECK_WRITING_MODE(aWritingMode);
    return mRect.height;
  }

  


  nscoord LineLeft(WritingMode aWritingMode, nscoord aContainerWidth) const
  {
    CHECK_WRITING_MODE(aWritingMode);
    if (aWritingMode.IsVertical()) {
      return IStart(); 
    } else {
      return aWritingMode.IsBidiLTR() ? IStart()
                                      : aContainerWidth - IEnd();
    }
  }
  nscoord LineRight(WritingMode aWritingMode, nscoord aContainerWidth) const
  {
    CHECK_WRITING_MODE(aWritingMode);
    if (aWritingMode.IsVertical()) {
      return IEnd(); 
    } else {
      return aWritingMode.IsBidiLTR() ? IEnd()
                                      : aContainerWidth - IStart();
    }
  }

  


  nscoord X(WritingMode aWritingMode, nscoord aContainerWidth) const
  {
    CHECK_WRITING_MODE(aWritingMode);
    if (aWritingMode.IsVertical()) {
      return aWritingMode.IsVerticalLR() ?
             mRect.Y() : aContainerWidth - mRect.YMost();
    } else {
      return aWritingMode.IsBidiLTR() ?
             mRect.X() : aContainerWidth - mRect.XMost();
    }
  }

  void SetX(WritingMode aWritingMode, nscoord aX, nscoord aContainerWidth)
  {
    CHECK_WRITING_MODE(aWritingMode);
    if (aWritingMode.IsVertical()) {
      if (aWritingMode.IsVerticalLR()) {
        BStart() = aX;
      } else {
        BStart() = aContainerWidth - aX - BSize();
      }
    } else {
      if (aWritingMode.IsBidiLTR()) {
        IStart() = aX;
      } else {
        IStart() = aContainerWidth - aX - ISize();
      }
    }
  }

  nscoord Y(WritingMode aWritingMode) const
  {
    CHECK_WRITING_MODE(aWritingMode);
    return aWritingMode.IsVertical() ? mRect.X() : mRect.Y();
  }

  void SetY(WritingMode aWritingMode, nscoord aY)
  {
    CHECK_WRITING_MODE(aWritingMode);
    if (aWritingMode.IsVertical()) {
      IStart() = aY;
    } else {
      BStart() = aY;
    }
  }

  nscoord Width(WritingMode aWritingMode) const
  {
    CHECK_WRITING_MODE(aWritingMode);
    return aWritingMode.IsVertical() ? mRect.Height() : mRect.Width();
  }

  
  
  
  void SetWidth(WritingMode aWritingMode, nscoord aWidth)
  {
    CHECK_WRITING_MODE(aWritingMode);
    if (aWritingMode.IsVertical()) {
      if (!aWritingMode.IsVerticalLR()) {
        BStart() = BStart() + BSize() - aWidth;
      }
      BSize() = aWidth;
    } else {
      if (!aWritingMode.IsBidiLTR()) {
        IStart() = IStart() + ISize() - aWidth;
      }
      ISize() = aWidth;
    }
  }

  nscoord Height(WritingMode aWritingMode) const
  {
    CHECK_WRITING_MODE(aWritingMode);
    return aWritingMode.IsVertical() ? mRect.Width() : mRect.Height();
  }

  void SetHeight(WritingMode aWritingMode, nscoord aHeight)
  {
    CHECK_WRITING_MODE(aWritingMode);
    if (aWritingMode.IsVertical()) {
      ISize() = aHeight;
    } else {
      BSize() = aHeight;
    }
  }

  nscoord XMost(WritingMode aWritingMode, nscoord aContainerWidth) const
  {
    CHECK_WRITING_MODE(aWritingMode);
    if (aWritingMode.IsVertical()) {
      return aWritingMode.IsVerticalLR() ?
             mRect.YMost() : aContainerWidth - mRect.Y();
    } else {
      return aWritingMode.IsBidiLTR() ?
             mRect.XMost() : aContainerWidth - mRect.X();
    }
  }

  nscoord YMost(WritingMode aWritingMode) const
  {
    CHECK_WRITING_MODE(aWritingMode);
    return aWritingMode.IsVertical() ? mRect.XMost() : mRect.YMost();
  }

  bool IsEmpty() const
  {
    return mRect.IsEmpty();
  }

  bool IsAllZero() const
  {
    return (mRect.x == 0 && mRect.y == 0 &&
            mRect.width == 0 && mRect.height == 0);
  }

  bool IsZeroSize() const
  {
    return (mRect.width == 0 && mRect.height == 0);
  }

  void SetEmpty() { mRect.SetEmpty(); }

  bool IsEqualEdges(const LogicalRect aOther) const
  {
    CHECK_WRITING_MODE(aOther.GetWritingMode());
    return mRect.IsEqualEdges(aOther.mRect);
  }

  LogicalPoint Origin(WritingMode aWritingMode) const
  {
    CHECK_WRITING_MODE(aWritingMode);
    return LogicalPoint(aWritingMode, IStart(), BStart());
  }
  void SetOrigin(WritingMode aWritingMode, const LogicalPoint& aPoint)
  {
    IStart(aWritingMode) = aPoint.I(aWritingMode);
    BStart(aWritingMode) = aPoint.B(aWritingMode);
  }

  LogicalSize Size(WritingMode aWritingMode) const
  {
    CHECK_WRITING_MODE(aWritingMode);
    return LogicalSize(aWritingMode, ISize(), BSize());
  }

  LogicalRect operator+(const LogicalPoint& aPoint) const
  {
    CHECK_WRITING_MODE(aPoint.GetWritingMode());
    return LogicalRect(GetWritingMode(),
                       IStart() + aPoint.I(), BStart() + aPoint.B(),
                       ISize(), BSize());
  }

  LogicalRect& operator+=(const LogicalPoint& aPoint)
  {
    CHECK_WRITING_MODE(aPoint.GetWritingMode());
    mRect += aPoint.mPoint;
    return *this;
  }

  LogicalRect operator-(const LogicalPoint& aPoint) const
  {
    CHECK_WRITING_MODE(aPoint.GetWritingMode());
    return LogicalRect(GetWritingMode(),
                       IStart() - aPoint.I(), BStart() - aPoint.B(),
                       ISize(), BSize());
  }

  LogicalRect& operator-=(const LogicalPoint& aPoint)
  {
    CHECK_WRITING_MODE(aPoint.GetWritingMode());
    mRect -= aPoint.mPoint;
    return *this;
  }

  void MoveBy(WritingMode aWritingMode, const LogicalPoint& aDelta)
  {
    CHECK_WRITING_MODE(aWritingMode);
    CHECK_WRITING_MODE(aDelta.GetWritingMode());
    IStart() += aDelta.I();
    BStart() += aDelta.B();
  }

  void Inflate(nscoord aD) { mRect.Inflate(aD); }
  void Inflate(nscoord aDI, nscoord aDB) { mRect.Inflate(aDI, aDB); }
  void Inflate(WritingMode aWritingMode, const LogicalMargin& aMargin)
  {
    CHECK_WRITING_MODE(aWritingMode);
    CHECK_WRITING_MODE(aMargin.GetWritingMode());
    mRect.Inflate(aMargin.mMargin);
  }

  void Deflate(nscoord aD) { mRect.Deflate(aD); }
  void Deflate(nscoord aDI, nscoord aDB) { mRect.Deflate(aDI, aDB); }
  void Deflate(WritingMode aWritingMode, const LogicalMargin& aMargin)
  {
    CHECK_WRITING_MODE(aWritingMode);
    CHECK_WRITING_MODE(aMargin.GetWritingMode());
    mRect.Deflate(aMargin.mMargin);
  }

  



  nsRect GetPhysicalRect(WritingMode aWritingMode,
                         nscoord aContainerWidth) const
  {
    CHECK_WRITING_MODE(aWritingMode);
    if (aWritingMode.IsVertical()) {
      return nsRect(aWritingMode.IsVerticalLR() ?
                      BStart() : aContainerWidth - BEnd(),
                    IStart(), BSize(), ISize());
    } else {
      return nsRect(aWritingMode.IsBidiLTR() ?
                      IStart() : aContainerWidth - IEnd(),
                    BStart(), ISize(), BSize());
    }
  }

  


  LogicalRect ConvertTo(WritingMode aToMode, WritingMode aFromMode,
                        nscoord aContainerWidth) const
  {
    CHECK_WRITING_MODE(aFromMode);
    return aToMode == aFromMode ?
      *this : LogicalRect(aToMode, GetPhysicalRect(aFromMode, aContainerWidth),
                          aContainerWidth);
  }

  



  bool IntersectRect(const LogicalRect& aRect1, const LogicalRect& aRect2)
  {
    CHECK_WRITING_MODE(aRect1.mWritingMode);
    CHECK_WRITING_MODE(aRect2.mWritingMode);
    return mRect.IntersectRect(aRect1.mRect, aRect2.mRect);
  }

private:
  LogicalRect() = delete;

#ifdef DEBUG
  WritingMode GetWritingMode() const { return mWritingMode; }
#else
  WritingMode GetWritingMode() const { return WritingMode::Unknown(); }
#endif

  nscoord IStart() const 
  {
    return mRect.X();
  }
  nscoord IEnd() const 
  {
    return mRect.XMost();
  }
  nscoord ISize() const 
  {
    return mRect.Width();
  }

  nscoord BStart() const 
  {
    return mRect.Y();
  }
  nscoord BEnd() const 
  {
    return mRect.YMost();
  }
  nscoord BSize() const 
  {
    return mRect.Height();
  }

  nscoord& IStart() 
  {
    return mRect.x;
  }
  nscoord& ISize() 
  {
    return mRect.width;
  }
  nscoord& BStart() 
  {
    return mRect.y;
  }
  nscoord& BSize() 
  {
    return mRect.height;
  }

#ifdef DEBUG
  WritingMode mWritingMode;
#endif
  nsRect      mRect;
};

} 

#endif 
