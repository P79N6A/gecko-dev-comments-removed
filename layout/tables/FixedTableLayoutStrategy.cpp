










#include "FixedTableLayoutStrategy.h"
#include "nsTableFrame.h"
#include "nsTableColFrame.h"
#include "nsTableCellFrame.h"
#include <algorithm>

FixedTableLayoutStrategy::FixedTableLayoutStrategy(nsTableFrame *aTableFrame)
  : nsITableLayoutStrategy(nsITableLayoutStrategy::Fixed)
  , mTableFrame(aTableFrame)
{
    MarkIntrinsicISizesDirty();
}


FixedTableLayoutStrategy::~FixedTableLayoutStrategy()
{
}

 nscoord
FixedTableLayoutStrategy::GetMinISize(nsRenderingContext* aRenderingContext)
{
    DISPLAY_MIN_WIDTH(mTableFrame, mMinISize);
    if (mMinISize != NS_INTRINSIC_WIDTH_UNKNOWN) {
        return mMinISize;
    }

    
    
    
    
    
    
    
    
    
    
    

    
    

    nsTableCellMap *cellMap = mTableFrame->GetCellMap();
    int32_t colCount = cellMap->GetColCount();

    nscoord result = 0;

    if (colCount > 0) {
        result += mTableFrame->GetColSpacing(-1, colCount);
    }

    WritingMode wm = mTableFrame->GetWritingMode();
    for (int32_t col = 0; col < colCount; ++col) {
        nsTableColFrame *colFrame = mTableFrame->GetColFrame(col);
        if (!colFrame) {
            NS_ERROR("column frames out of sync with cell map");
            continue;
        }
        nscoord spacing = mTableFrame->GetColSpacing(col);
        const nsStyleCoord *styleISize = &colFrame->StylePosition()->ISize(wm);
        if (styleISize->ConvertsToLength()) {
            result += nsLayoutUtils::ComputeISizeValue(aRenderingContext,
                        colFrame, 0, 0, 0, *styleISize);
        } else if (styleISize->GetUnit() == eStyleUnit_Percent) {
            
        } else {
            NS_ASSERTION(styleISize->GetUnit() == eStyleUnit_Auto ||
                         styleISize->GetUnit() == eStyleUnit_Enumerated ||
                         (styleISize->IsCalcUnit() && styleISize->CalcHasPercent()),
                         "bad inline size");

            
            
            bool originates;
            int32_t colSpan;
            nsTableCellFrame *cellFrame =
                cellMap->GetCellInfoAt(0, col, &originates, &colSpan);
            if (cellFrame) {
                styleISize = &cellFrame->StylePosition()->ISize(wm);
                if (styleISize->ConvertsToLength() ||
                    (styleISize->GetUnit() == eStyleUnit_Enumerated &&
                     (styleISize->GetIntValue() == NS_STYLE_WIDTH_MAX_CONTENT ||
                      styleISize->GetIntValue() ==
                      NS_STYLE_WIDTH_MIN_CONTENT))) {
                    nscoord cellISize = nsLayoutUtils::IntrinsicForContainer(
                        aRenderingContext, cellFrame, nsLayoutUtils::MIN_ISIZE);
                    if (colSpan > 1) {
                        
                        
                        
                        
                        
                        cellISize = ((cellISize + spacing) / colSpan) - spacing;
                    }
                    result += cellISize;
                } else if (styleISize->GetUnit() == eStyleUnit_Percent) {
                    if (colSpan > 1) {
                        
                        
                        result -= spacing * (colSpan - 1);
                    }
                }
                
                
            }
        }
    }

    return (mMinISize = result);
}

 nscoord
FixedTableLayoutStrategy::GetPrefISize(nsRenderingContext* aRenderingContext,
                                       bool aComputingSize)
{
    
    
    
    
    
    
    nscoord result = nscoord_MAX;
    DISPLAY_PREF_WIDTH(mTableFrame, result);
    return result;
}

 void
FixedTableLayoutStrategy::MarkIntrinsicISizesDirty()
{
    mMinISize = NS_INTRINSIC_WIDTH_UNKNOWN;
    mLastCalcISize = nscoord_MIN;
}

static inline nscoord
AllocateUnassigned(nscoord aUnassignedSpace, float aShare)
{
    if (aShare == 1.0f) {
        
        
        
        return aUnassignedSpace;
    }
    return NSToCoordRound(float(aUnassignedSpace) * aShare);
}

 void
FixedTableLayoutStrategy::ComputeColumnISizes(const nsHTMLReflowState& aReflowState)
{
    nscoord tableISize = aReflowState.ComputedISize();

    if (mLastCalcISize == tableISize) {
        return;
    }
    mLastCalcISize = tableISize;

    nsTableCellMap *cellMap = mTableFrame->GetCellMap();
    int32_t colCount = cellMap->GetColCount();

    if (colCount == 0) {
        
        return;
    }

    
    tableISize -= mTableFrame->GetColSpacing(-1, colCount);

    
    
    
    
    
    
    
    nsTArray<nscoord> oldColISizes;

    
    

    

    uint32_t unassignedCount = 0;
    nscoord unassignedSpace = tableISize;
    const nscoord unassignedMarker = nscoord_MIN;

    
    
    
    float pctTotal = 0.0f;

    
    
    nscoord specTotal = 0;

    WritingMode wm = mTableFrame->GetWritingMode();
    for (int32_t col = 0; col < colCount; ++col) {
        nsTableColFrame *colFrame = mTableFrame->GetColFrame(col);
        if (!colFrame) {
            oldColISizes.AppendElement(0);
            NS_ERROR("column frames out of sync with cell map");
            continue;
        }
        oldColISizes.AppendElement(colFrame->GetFinalISize());
        colFrame->ResetPrefPercent();
        const nsStyleCoord *styleISize = &colFrame->StylePosition()->ISize(wm);
        nscoord colISize;
        if (styleISize->ConvertsToLength()) {
            colISize = nsLayoutUtils::ComputeISizeValue(
                         aReflowState.rendContext,
                         colFrame, 0, 0, 0, *styleISize);
            specTotal += colISize;
        } else if (styleISize->GetUnit() == eStyleUnit_Percent) {
            float pct = styleISize->GetPercentValue();
            colISize = NSToCoordFloor(pct * float(tableISize));
            colFrame->AddPrefPercent(pct);
            pctTotal += pct;
        } else {
            NS_ASSERTION(styleISize->GetUnit() == eStyleUnit_Auto ||
                         styleISize->GetUnit() == eStyleUnit_Enumerated ||
                         (styleISize->IsCalcUnit() &&
                          styleISize->CalcHasPercent()),
                         "bad inline size");

            
            
            bool originates;
            int32_t colSpan;
            nsTableCellFrame *cellFrame =
                cellMap->GetCellInfoAt(0, col, &originates, &colSpan);
            if (cellFrame) {
                styleISize = &cellFrame->StylePosition()->ISize(wm);
                if (styleISize->ConvertsToLength() ||
                    (styleISize->GetUnit() == eStyleUnit_Enumerated &&
                     (styleISize->GetIntValue() == NS_STYLE_WIDTH_MAX_CONTENT ||
                      styleISize->GetIntValue() ==
                      NS_STYLE_WIDTH_MIN_CONTENT))) {
                    
                    
                    
                    
                    
                    
                    colISize = nsLayoutUtils::IntrinsicForContainer(
                                 aReflowState.rendContext,
                                 cellFrame, nsLayoutUtils::MIN_ISIZE);
                } else if (styleISize->GetUnit() == eStyleUnit_Percent) {
                    
                    nsIFrame::IntrinsicISizeOffsetData offsets =
                        cellFrame->IntrinsicISizeOffsets();
                    float pct = styleISize->GetPercentValue();
                    colISize = NSToCoordFloor(pct * float(tableISize));

                    nscoord boxSizingAdjust = 0;
                    switch (cellFrame->StylePosition()->mBoxSizing) {
                      case NS_STYLE_BOX_SIZING_CONTENT:
                        boxSizingAdjust += offsets.hPadding;
                        
                      case NS_STYLE_BOX_SIZING_PADDING:
                        boxSizingAdjust += offsets.hBorder;
                        
                      case NS_STYLE_BOX_SIZING_BORDER:
                        
                        break;
                    }
                    colISize += boxSizingAdjust;

                    pct /= float(colSpan);
                    colFrame->AddPrefPercent(pct);
                    pctTotal += pct;
                } else {
                    
                    
                    colISize = unassignedMarker;
                }
                if (colISize != unassignedMarker) {
                    if (colSpan > 1) {
                        
                        
                        
                        
                        nscoord spacing = mTableFrame->GetColSpacing(col);
                        colISize = ((colISize + spacing) / colSpan) - spacing;
                        if (colISize < 0)
                            colISize = 0;
                    }
                    if (styleISize->GetUnit() != eStyleUnit_Percent) {
                        specTotal += colISize;
                    }
                }
            } else {
                colISize = unassignedMarker;
            }
        }

        colFrame->SetFinalISize(colISize);

        if (colISize == unassignedMarker) {
            ++unassignedCount;
        } else {
            unassignedSpace -= colISize;
        }
    }

    if (unassignedSpace < 0) {
        if (pctTotal > 0) {
            
            
            
            
            nscoord pctUsed = NSToCoordFloor(pctTotal * float(tableISize));
            nscoord reduce = std::min(pctUsed, -unassignedSpace);
            float reduceRatio = float(reduce) / pctTotal;
            for (int32_t col = 0; col < colCount; ++col) {
                nsTableColFrame *colFrame = mTableFrame->GetColFrame(col);
                if (!colFrame) {
                    NS_ERROR("column frames out of sync with cell map");
                    continue;
                }
                nscoord colISize = colFrame->GetFinalISize();
                colISize -= NSToCoordFloor(colFrame->GetPrefPercent() *
                                           reduceRatio);
                if (colISize < 0)
                    colISize = 0;
                colFrame->SetFinalISize(colISize);
            }
        }
        unassignedSpace = 0;
    }

    if (unassignedCount > 0) {
        
        
        nscoord toAssign = unassignedSpace / unassignedCount;
        for (int32_t col = 0; col < colCount; ++col) {
            nsTableColFrame *colFrame = mTableFrame->GetColFrame(col);
            if (!colFrame) {
                NS_ERROR("column frames out of sync with cell map");
                continue;
            }
            if (colFrame->GetFinalISize() == unassignedMarker)
                colFrame->SetFinalISize(toAssign);
        }
    } else if (unassignedSpace > 0) {
        
        if (specTotal > 0) {
            
            nscoord specUndist = specTotal;
            for (int32_t col = 0; col < colCount; ++col) {
                nsTableColFrame *colFrame = mTableFrame->GetColFrame(col);
                if (!colFrame) {
                    NS_ERROR("column frames out of sync with cell map");
                    continue;
                }
                if (colFrame->GetPrefPercent() == 0.0f) {
                    NS_ASSERTION(colFrame->GetFinalISize() <= specUndist,
                                 "inline sizes don't add up");
                    nscoord toAdd = AllocateUnassigned(unassignedSpace,
                       float(colFrame->GetFinalISize()) / float(specUndist));
                    specUndist -= colFrame->GetFinalISize();
                    colFrame->SetFinalISize(colFrame->GetFinalISize() + toAdd);
                    unassignedSpace -= toAdd;
                    if (specUndist <= 0) {
                        NS_ASSERTION(specUndist == 0,
                                     "math should be exact");
                        break;
                    }
                }
            }
            NS_ASSERTION(unassignedSpace == 0, "failed to redistribute");
        } else if (pctTotal > 0) {
            
            float pctUndist = pctTotal;
            for (int32_t col = 0; col < colCount; ++col) {
                nsTableColFrame *colFrame = mTableFrame->GetColFrame(col);
                if (!colFrame) {
                    NS_ERROR("column frames out of sync with cell map");
                    continue;
                }
                if (pctUndist < colFrame->GetPrefPercent()) {
                    
                    NS_ASSERTION(colFrame->GetPrefPercent() - pctUndist
                                   < 0.0001,
                                 "inline sizes don't add up");
                    pctUndist = colFrame->GetPrefPercent();
                }
                nscoord toAdd = AllocateUnassigned(unassignedSpace,
                    colFrame->GetPrefPercent() / pctUndist);
                colFrame->SetFinalISize(colFrame->GetFinalISize() + toAdd);
                unassignedSpace -= toAdd;
                pctUndist -= colFrame->GetPrefPercent();
                if (pctUndist <= 0.0f) {
                    break;
                }
            }
            NS_ASSERTION(unassignedSpace == 0, "failed to redistribute");
        } else {
            
            int32_t colsRemaining = colCount;
            for (int32_t col = 0; col < colCount; ++col) {
                nsTableColFrame *colFrame = mTableFrame->GetColFrame(col);
                if (!colFrame) {
                    NS_ERROR("column frames out of sync with cell map");
                    continue;
                }
                NS_ASSERTION(colFrame->GetFinalISize() == 0, "yikes");
                nscoord toAdd = AllocateUnassigned(unassignedSpace,
                                                   1.0f / float(colsRemaining));
                colFrame->SetFinalISize(toAdd);
                unassignedSpace -= toAdd;
                --colsRemaining;
            }
            NS_ASSERTION(unassignedSpace == 0, "failed to redistribute");
        }
    }
    for (int32_t col = 0; col < colCount; ++col) {
        nsTableColFrame *colFrame = mTableFrame->GetColFrame(col);
        if (!colFrame) {
            NS_ERROR("column frames out of sync with cell map");
            continue;
        }
        if (oldColISizes.ElementAt(col) != colFrame->GetFinalISize()) {
            mTableFrame->DidResizeColumns();
            break;
        }
    }
}
