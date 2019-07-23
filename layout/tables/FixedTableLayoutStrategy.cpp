










































#include "FixedTableLayoutStrategy.h"
#include "nsTableFrame.h"
#include "nsTableColFrame.h"
#include "nsTableCellFrame.h"

FixedTableLayoutStrategy::FixedTableLayoutStrategy(nsTableFrame *aTableFrame)
  : nsITableLayoutStrategy(nsITableLayoutStrategy::Fixed)
  , mTableFrame(aTableFrame)
{
    MarkIntrinsicWidthsDirty();
}


FixedTableLayoutStrategy::~FixedTableLayoutStrategy()
{
}

 nscoord
FixedTableLayoutStrategy::GetMinWidth(nsIRenderingContext* aRenderingContext)
{
    DISPLAY_MIN_WIDTH(mTableFrame, mMinWidth);
    if (mMinWidth != NS_INTRINSIC_WIDTH_UNKNOWN)
        return mMinWidth;

    
    
    
    
    
    
    
    
    
    
    

    
    

    nsTableCellMap *cellMap = mTableFrame->GetCellMap();
    PRInt32 colCount = cellMap->GetColCount();
    nscoord spacing = mTableFrame->GetCellSpacingX();

    nscoord result = 0;

    if (colCount > 0) {
        result += spacing * (colCount + 1);
    }

    for (PRInt32 col = 0; col < colCount; ++col) {
        nsTableColFrame *colFrame = mTableFrame->GetColFrame(col);
        if (!colFrame) {
            NS_ERROR("column frames out of sync with cell map");
            continue;
        }
        const nsStyleCoord *styleWidth =
            &colFrame->GetStylePosition()->mWidth;
        if (styleWidth->GetUnit() == eStyleUnit_Coord) {
            result += nsLayoutUtils::ComputeWidthValue(aRenderingContext,
                        colFrame, 0, 0, 0, *styleWidth);
        } else if (styleWidth->GetUnit() == eStyleUnit_Percent) {
            
        } else {
            NS_ASSERTION(styleWidth->GetUnit() == eStyleUnit_Auto ||
                         styleWidth->GetUnit() == eStyleUnit_Enumerated,
                         "bad width");

            
            
            PRBool originates;
            PRInt32 colSpan;
            nsTableCellFrame *cellFrame =
                cellMap->GetCellInfoAt(0, col, &originates, &colSpan);
            if (cellFrame) {
                styleWidth = &cellFrame->GetStylePosition()->mWidth;
                if (styleWidth->GetUnit() == eStyleUnit_Coord ||
                    (styleWidth->GetUnit() == eStyleUnit_Enumerated &&
                     (styleWidth->GetIntValue() == NS_STYLE_WIDTH_MAX_CONTENT ||
                      styleWidth->GetIntValue() == NS_STYLE_WIDTH_MIN_CONTENT))) {
                    nscoord cellWidth = nsLayoutUtils::IntrinsicForContainer(
                        aRenderingContext, cellFrame, nsLayoutUtils::MIN_WIDTH);
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
FixedTableLayoutStrategy::GetPrefWidth(nsIRenderingContext* aRenderingContext,
                                       PRBool aComputingSize)
{
    
    
    
    
    
    
    nscoord result = nscoord_MAX;
    DISPLAY_PREF_WIDTH(mTableFrame, result);
    return result;
}

 void
FixedTableLayoutStrategy::MarkIntrinsicWidthsDirty()
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
FixedTableLayoutStrategy::ComputeColumnWidths(const nsHTMLReflowState& aReflowState)
{
    nscoord tableWidth = aReflowState.ComputedWidth();

    if (mLastCalcWidth == tableWidth)
        return;
    mLastCalcWidth = tableWidth;

    nsTableCellMap *cellMap = mTableFrame->GetCellMap();
    PRInt32 colCount = cellMap->GetColCount();
    nscoord spacing = mTableFrame->GetCellSpacingX();

    if (colCount == 0) {
        
        return;
    }

    
    tableWidth -= spacing * (colCount + 1);
    
    
    
    
    
    
    
    nsTArray<nscoord> oldColWidths;

    
    

    

    PRUint32 unassignedCount = 0;
    nscoord unassignedSpace = tableWidth;
    const nscoord unassignedMarker = nscoord_MIN;

    
    
    
    float pctTotal = 0.0f;

    
    
    nscoord specTotal = 0;

    for (PRInt32 col = 0; col < colCount; ++col) {
        nsTableColFrame *colFrame = mTableFrame->GetColFrame(col);
        if (!colFrame) {
            oldColWidths.AppendElement(0);
            NS_ERROR("column frames out of sync with cell map");
            continue;
        }
        oldColWidths.AppendElement(colFrame->GetFinalWidth());
        colFrame->ResetPrefPercent();
        const nsStyleCoord *styleWidth =
            &colFrame->GetStylePosition()->mWidth;
        nscoord colWidth;
        if (styleWidth->GetUnit() == eStyleUnit_Coord) {
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
                         styleWidth->GetUnit() == eStyleUnit_Enumerated,
                         "bad width");

            
            
            PRBool originates;
            PRInt32 colSpan;
            nsTableCellFrame *cellFrame =
                cellMap->GetCellInfoAt(0, col, &originates, &colSpan);
            if (cellFrame) {
                styleWidth = &cellFrame->GetStylePosition()->mWidth;
                if (styleWidth->GetUnit() == eStyleUnit_Coord ||
                    (styleWidth->GetUnit() == eStyleUnit_Enumerated &&
                     (styleWidth->GetIntValue() == NS_STYLE_WIDTH_MAX_CONTENT ||
                      styleWidth->GetIntValue() == NS_STYLE_WIDTH_MIN_CONTENT))) {
                    
                    
                    
                    
                    
                    
                    colWidth = nsLayoutUtils::IntrinsicForContainer(
                                 aReflowState.rendContext,
                                 cellFrame, nsLayoutUtils::MIN_WIDTH);
                } else if (styleWidth->GetUnit() == eStyleUnit_Percent) {
                    
                    nsIFrame::IntrinsicWidthOffsetData offsets =
                        cellFrame->IntrinsicWidthOffsets(aReflowState.rendContext);
                    float pct = styleWidth->GetPercentValue();
                    colWidth = NSToCoordFloor(pct * float(tableWidth)) +
                               offsets.hPadding + offsets.hBorder;
                    pct /= float(colSpan);
                    colFrame->AddPrefPercent(pct);
                    pctTotal += pct;
                } else {
                    
                    colWidth = unassignedMarker;
                }
                if (colWidth != unassignedMarker) {
                    if (colSpan > 1) {
                        
                        
                        
                        
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
            nscoord reduce = NS_MIN(pctUsed, -unassignedSpace);
            float reduceRatio = float(reduce) / pctTotal;
            for (PRInt32 col = 0; col < colCount; ++col) {
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
        for (PRInt32 col = 0; col < colCount; ++col) {
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
            for (PRInt32 col = 0; col < colCount; ++col) {
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
            for (PRInt32 col = 0; col < colCount; ++col) {
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
            
            PRInt32 colsLeft = colCount;
            for (PRInt32 col = 0; col < colCount; ++col) {
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
    for (PRInt32 col = 0; col < colCount; ++col) {
        nsTableColFrame *colFrame = mTableFrame->GetColFrame(col);
        if (!colFrame) {
            NS_ERROR("column frames out of sync with cell map");
            continue;
        }
        if (oldColWidths.ElementAt(col) != colFrame->GetFinalWidth()) {
            mTableFrame->DidResizeColumns();
        }
            break;
    }
}
