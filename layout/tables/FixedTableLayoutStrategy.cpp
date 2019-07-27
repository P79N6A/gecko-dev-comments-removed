










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
    DISPLAY_MIN_WIDTH(mTableFrame, mMinWidth);
    if (mMinWidth != NS_INTRINSIC_WIDTH_UNKNOWN)
        return mMinWidth;

    
    
    
    
    
    
    
    
    
    
    

    
    

    nsTableCellMap *cellMap = mTableFrame->GetCellMap();
    int32_t colCount = cellMap->GetColCount();

    nscoord result = 0;

    if (colCount > 0) {
        result += mTableFrame->GetColSpacing(-1, colCount);
    }

    for (int32_t col = 0; col < colCount; ++col) {
        nsTableColFrame *colFrame = mTableFrame->GetColFrame(col);
        if (!colFrame) {
            NS_ERROR("column frames out of sync with cell map");
            continue;
        }
        nscoord spacing = mTableFrame->GetColSpacing(col);
        const nsStyleCoord *styleWidth =
            &colFrame->StylePosition()->mWidth;
        if (styleWidth->ConvertsToLength()) {
            result += nsLayoutUtils::ComputeWidthValue(aRenderingContext,
                        colFrame, 0, 0, 0, *styleWidth);
        } else if (styleWidth->GetUnit() == eStyleUnit_Percent) {
            
        } else {
            NS_ASSERTION(styleWidth->GetUnit() == eStyleUnit_Auto ||
                         styleWidth->GetUnit() == eStyleUnit_Enumerated ||
                         (styleWidth->IsCalcUnit() && styleWidth->CalcHasPercent()),
                         "bad width");

            
            
            bool originates;
            int32_t colSpan;
            nsTableCellFrame *cellFrame =
                cellMap->GetCellInfoAt(0, col, &originates, &colSpan);
            if (cellFrame) {
                styleWidth = &cellFrame->StylePosition()->mWidth;
                if (styleWidth->ConvertsToLength() ||
                    (styleWidth->GetUnit() == eStyleUnit_Enumerated &&
                     (styleWidth->GetIntValue() == NS_STYLE_WIDTH_MAX_CONTENT ||
                      styleWidth->GetIntValue() == NS_STYLE_WIDTH_MIN_CONTENT))) {
                    nscoord cellWidth = nsLayoutUtils::IntrinsicForContainer(
                        aRenderingContext, cellFrame, nsLayoutUtils::MIN_ISIZE);
                    if (colSpan > 1) {
                        
                        
                        
                        
                        cellWidth = ((cellWidth + spacing) / colSpan) - spacing;
                    }
                    result += cellWidth;
                } else if (styleWidth->GetUnit() == eStyleUnit_Percent) {
                    if (colSpan > 1) {
                        
                        
                        result -= spacing * (colSpan - 1);
                    }
                }
                
                
            }
        }
    }

    return (mMinWidth = result);
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
    mMinWidth = NS_INTRINSIC_WIDTH_UNKNOWN;
    mLastCalcWidth = nscoord_MIN;
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
    nscoord tableWidth = aReflowState.ComputedWidth();

    if (mLastCalcWidth == tableWidth)
        return;
    mLastCalcWidth = tableWidth;

    nsTableCellMap *cellMap = mTableFrame->GetCellMap();
    int32_t colCount = cellMap->GetColCount();

    if (colCount == 0) {
        
        return;
    }

    
    tableWidth -= mTableFrame->GetColSpacing(-1, colCount);

    
    
    
    
    
    
    nsTArray<nscoord> oldColWidths;

    
    

    

    uint32_t unassignedCount = 0;
    nscoord unassignedSpace = tableWidth;
    const nscoord unassignedMarker = nscoord_MIN;

    
    
    
    float pctTotal = 0.0f;

    
    
    nscoord specTotal = 0;

    for (int32_t col = 0; col < colCount; ++col) {
        nsTableColFrame *colFrame = mTableFrame->GetColFrame(col);
        if (!colFrame) {
            oldColWidths.AppendElement(0);
            NS_ERROR("column frames out of sync with cell map");
            continue;
        }
        oldColWidths.AppendElement(colFrame->GetFinalWidth());
        colFrame->ResetPrefPercent();
        const nsStyleCoord *styleWidth =
            &colFrame->StylePosition()->mWidth;
        nscoord colWidth;
        if (styleWidth->ConvertsToLength()) {
            colWidth = nsLayoutUtils::ComputeWidthValue(
                         aReflowState.rendContext,
                         colFrame, 0, 0, 0, *styleWidth);
            specTotal += colWidth;
        } else if (styleWidth->GetUnit() == eStyleUnit_Percent) {
            float pct = styleWidth->GetPercentValue();
            colWidth = NSToCoordFloor(pct * float(tableWidth));
            colFrame->AddPrefPercent(pct);
            pctTotal += pct;
        } else {
            NS_ASSERTION(styleWidth->GetUnit() == eStyleUnit_Auto ||
                         styleWidth->GetUnit() == eStyleUnit_Enumerated ||
                         (styleWidth->IsCalcUnit() && styleWidth->CalcHasPercent()),
                         "bad width");

            
            
            bool originates;
            int32_t colSpan;
            nsTableCellFrame *cellFrame =
                cellMap->GetCellInfoAt(0, col, &originates, &colSpan);
            if (cellFrame) {
                styleWidth = &cellFrame->StylePosition()->mWidth;
                if (styleWidth->ConvertsToLength() ||
                    (styleWidth->GetUnit() == eStyleUnit_Enumerated &&
                     (styleWidth->GetIntValue() == NS_STYLE_WIDTH_MAX_CONTENT ||
                      styleWidth->GetIntValue() == NS_STYLE_WIDTH_MIN_CONTENT))) {
                    
                    
                    
                    
                    
                    
                    colWidth = nsLayoutUtils::IntrinsicForContainer(
                                 aReflowState.rendContext,
                                 cellFrame, nsLayoutUtils::MIN_ISIZE);
                } else if (styleWidth->GetUnit() == eStyleUnit_Percent) {
                    
                    nsIFrame::IntrinsicISizeOffsetData offsets =
                        cellFrame->IntrinsicISizeOffsets(aReflowState.rendContext);
                    float pct = styleWidth->GetPercentValue();
                    colWidth = NSToCoordFloor(pct * float(tableWidth));

                    nscoord boxSizingAdjust = 0;
                    switch (cellFrame->StylePosition()->mBoxSizing) {
                      case NS_STYLE_BOX_SIZING_CONTENT:
                        boxSizingAdjust += offsets.hPadding;
                        
                      case NS_STYLE_BOX_SIZING_PADDING:
                        boxSizingAdjust += offsets.hBorder;
                        
                      case NS_STYLE_BOX_SIZING_BORDER:
                        
                        break;
                    }
                    colWidth += boxSizingAdjust;

                    pct /= float(colSpan);
                    colFrame->AddPrefPercent(pct);
                    pctTotal += pct;
                } else {
                    
                    
                    colWidth = unassignedMarker;
                }
                if (colWidth != unassignedMarker) {
                    if (colSpan > 1) {
                        
                        
                        
                        
                        nscoord spacing = mTableFrame->GetColSpacing(col);
                        colWidth = ((colWidth + spacing) / colSpan) - spacing;
                        if (colWidth < 0)
                            colWidth = 0;
                    }
                    if (styleWidth->GetUnit() != eStyleUnit_Percent) {
                        specTotal += colWidth;
                    }
                }
            } else {
                colWidth = unassignedMarker;
            }
        }

        colFrame->SetFinalWidth(colWidth);

        if (colWidth == unassignedMarker) {
            ++unassignedCount;
        } else {
            unassignedSpace -= colWidth;
        }
    }

    if (unassignedSpace < 0) {
        if (pctTotal > 0) {
            
            
            
            nscoord pctUsed = NSToCoordFloor(pctTotal * float(tableWidth));
            nscoord reduce = std::min(pctUsed, -unassignedSpace);
            float reduceRatio = float(reduce) / pctTotal;
            for (int32_t col = 0; col < colCount; ++col) {
                nsTableColFrame *colFrame = mTableFrame->GetColFrame(col);
                if (!colFrame) {
                    NS_ERROR("column frames out of sync with cell map");
                    continue;
                }
                nscoord colWidth = colFrame->GetFinalWidth();
                colWidth -= NSToCoordFloor(colFrame->GetPrefPercent() *
                                           reduceRatio);
                if (colWidth < 0)
                    colWidth = 0;
                colFrame->SetFinalWidth(colWidth);
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
            if (colFrame->GetFinalWidth() == unassignedMarker)
                colFrame->SetFinalWidth(toAssign);
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
                    NS_ASSERTION(colFrame->GetFinalWidth() <= specUndist,
                                 "widths don't add up");
                    nscoord toAdd = AllocateUnassigned(unassignedSpace,
                       float(colFrame->GetFinalWidth()) / float(specUndist));
                    specUndist -= colFrame->GetFinalWidth();
                    colFrame->SetFinalWidth(colFrame->GetFinalWidth() + toAdd);
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
                                 "widths don't add up");
                    pctUndist = colFrame->GetPrefPercent();
                }
                nscoord toAdd = AllocateUnassigned(unassignedSpace,
                    colFrame->GetPrefPercent() / pctUndist);
                colFrame->SetFinalWidth(colFrame->GetFinalWidth() + toAdd);
                unassignedSpace -= toAdd;
                pctUndist -= colFrame->GetPrefPercent();
                if (pctUndist <= 0.0f) {
                    break;
                }
            }
            NS_ASSERTION(unassignedSpace == 0, "failed to redistribute");
        } else {
            
            int32_t colsLeft = colCount;
            for (int32_t col = 0; col < colCount; ++col) {
                nsTableColFrame *colFrame = mTableFrame->GetColFrame(col);
                if (!colFrame) {
                    NS_ERROR("column frames out of sync with cell map");
                    continue;
                }
                NS_ASSERTION(colFrame->GetFinalWidth() == 0, "yikes");
                nscoord toAdd = AllocateUnassigned(unassignedSpace,
                                                   1.0f / float(colsLeft));
                colFrame->SetFinalWidth(toAdd);
                unassignedSpace -= toAdd;
                --colsLeft;
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
        if (oldColWidths.ElementAt(col) != colFrame->GetFinalWidth()) {
            mTableFrame->DidResizeColumns();
            break;
        }
    }
}
