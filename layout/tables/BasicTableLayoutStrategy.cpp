










#include "BasicTableLayoutStrategy.h"

#include <algorithm>

#include "nsTableFrame.h"
#include "nsTableColFrame.h"
#include "nsTableCellFrame.h"
#include "nsLayoutUtils.h"
#include "nsGkAtoms.h"
#include "SpanningCellSorter.h"
#include "nsIContent.h"

using namespace mozilla;
using namespace mozilla::layout;

namespace css = mozilla::css;

#undef  DEBUG_TABLE_STRATEGY 

BasicTableLayoutStrategy::BasicTableLayoutStrategy(nsTableFrame *aTableFrame)
  : nsITableLayoutStrategy(nsITableLayoutStrategy::Auto)
  , mTableFrame(aTableFrame)
{
    MarkIntrinsicISizesDirty();
}


BasicTableLayoutStrategy::~BasicTableLayoutStrategy()
{
}

 nscoord
BasicTableLayoutStrategy::GetMinISize(nsRenderingContext* aRenderingContext)
{
    DISPLAY_MIN_WIDTH(mTableFrame, mMinWidth);
    if (mMinWidth == NS_INTRINSIC_WIDTH_UNKNOWN)
        ComputeIntrinsicISizes(aRenderingContext);
    return mMinWidth;
}

 nscoord
BasicTableLayoutStrategy::GetPrefISize(nsRenderingContext* aRenderingContext,
                                       bool aComputingSize)
{
    DISPLAY_PREF_WIDTH(mTableFrame, mPrefWidth);
    NS_ASSERTION((mPrefWidth == NS_INTRINSIC_WIDTH_UNKNOWN) ==
                 (mPrefWidthPctExpand == NS_INTRINSIC_WIDTH_UNKNOWN),
                 "dirtyness out of sync");
    if (mPrefWidth == NS_INTRINSIC_WIDTH_UNKNOWN)
        ComputeIntrinsicISizes(aRenderingContext);
    return aComputingSize ? mPrefWidthPctExpand : mPrefWidth;
}

struct CellWidthInfo {
    CellWidthInfo(nscoord aMinCoord, nscoord aPrefCoord,
                  float aPrefPercent, bool aHasSpecifiedWidth)
        : hasSpecifiedWidth(aHasSpecifiedWidth)
        , minCoord(aMinCoord)
        , prefCoord(aPrefCoord)
        , prefPercent(aPrefPercent)
    {
    }

    bool hasSpecifiedWidth;
    nscoord minCoord;
    nscoord prefCoord;
    float prefPercent;
};



static CellWidthInfo
GetWidthInfo(nsRenderingContext *aRenderingContext,
             nsIFrame *aFrame, bool aIsCell)
{
    nscoord minCoord, prefCoord;
    const nsStylePosition *stylePos = aFrame->StylePosition();
    bool isQuirks = aFrame->PresContext()->CompatibilityMode() ==
                    eCompatibility_NavQuirks;
    nscoord boxSizingToBorderEdge = 0;
    if (aIsCell) {
        
        
        AutoMaybeDisableFontInflation an(aFrame);

        minCoord = aFrame->GetMinISize(aRenderingContext);
        prefCoord = aFrame->GetPrefISize(aRenderingContext);
        
        
        
        
        
        

        
        nsIFrame::IntrinsicISizeOffsetData offsets = aFrame->IntrinsicISizeOffsets(aRenderingContext);

        
        
        
        
        
        
        
        if (isQuirks) {
            boxSizingToBorderEdge = offsets.hPadding + offsets.hBorder;
        }
        else {
            switch (stylePos->mBoxSizing) {
                case NS_STYLE_BOX_SIZING_CONTENT:
                    boxSizingToBorderEdge = offsets.hPadding + offsets.hBorder;
                    break;
                case NS_STYLE_BOX_SIZING_PADDING:
                    minCoord += offsets.hPadding;
                    prefCoord += offsets.hPadding;
                    boxSizingToBorderEdge = offsets.hBorder;
                    break;
                default:
                    
                    minCoord += offsets.hPadding + offsets.hBorder;
                    prefCoord += offsets.hPadding + offsets.hBorder;
                    break;
            }
        }
    } else {
        minCoord = 0;
        prefCoord = 0;
    }
    float prefPercent = 0.0f;
    bool hasSpecifiedWidth = false;

    const nsStyleCoord &width = stylePos->mWidth;
    nsStyleUnit unit = width.GetUnit();
    
    
    
    
    if (width.ConvertsToLength()) {
        hasSpecifiedWidth = true;
        
        
        
        
        nscoord w = nsLayoutUtils::ComputeWidthValue(aRenderingContext,
                                                     aFrame, 0, 0, 0, width);
        
        
        
        
        
        if (aIsCell && w > minCoord && isQuirks &&
            aFrame->GetContent()->HasAttr(kNameSpaceID_None,
                                          nsGkAtoms::nowrap)) {
            minCoord = w;
        }
        prefCoord = std::max(w, minCoord);
    } else if (unit == eStyleUnit_Percent) {
        prefPercent = width.GetPercentValue();
    } else if (unit == eStyleUnit_Enumerated && aIsCell) {
        switch (width.GetIntValue()) {
            case NS_STYLE_WIDTH_MAX_CONTENT:
                
                
                break;
            case NS_STYLE_WIDTH_MIN_CONTENT:
                prefCoord = minCoord;
                break;
            case NS_STYLE_WIDTH_FIT_CONTENT:
            case NS_STYLE_WIDTH_AVAILABLE:
                
                break;
            default:
                NS_NOTREACHED("unexpected enumerated value");
        }
    }

    nsStyleCoord maxWidth(stylePos->mMaxWidth);
    if (maxWidth.GetUnit() == eStyleUnit_Enumerated) {
        if (!aIsCell || maxWidth.GetIntValue() == NS_STYLE_WIDTH_AVAILABLE)
            maxWidth.SetNoneValue();
        else if (maxWidth.GetIntValue() == NS_STYLE_WIDTH_FIT_CONTENT)
            
            
            maxWidth.SetIntValue(NS_STYLE_WIDTH_MAX_CONTENT,
                                 eStyleUnit_Enumerated);
    }
    unit = maxWidth.GetUnit();
    
    
    if (maxWidth.ConvertsToLength() || unit == eStyleUnit_Enumerated) {
        nscoord w =
            nsLayoutUtils::ComputeWidthValue(aRenderingContext, aFrame,
                                             0, 0, 0, maxWidth);
        if (w < minCoord)
            minCoord = w;
        if (w < prefCoord)
            prefCoord = w;
    } else if (unit == eStyleUnit_Percent) {
        float p = stylePos->mMaxWidth.GetPercentValue();
        if (p < prefPercent)
            prefPercent = p;
    }
    

    nsStyleCoord minWidth(stylePos->mMinWidth);
    if (minWidth.GetUnit() == eStyleUnit_Enumerated) {
        if (!aIsCell || minWidth.GetIntValue() == NS_STYLE_WIDTH_AVAILABLE)
            minWidth.SetCoordValue(0);
        else if (minWidth.GetIntValue() == NS_STYLE_WIDTH_FIT_CONTENT)
            
            
            minWidth.SetIntValue(NS_STYLE_WIDTH_MIN_CONTENT,
                                 eStyleUnit_Enumerated);
    }
    unit = minWidth.GetUnit();
    if (minWidth.ConvertsToLength() || unit == eStyleUnit_Enumerated) {
        nscoord w =
            nsLayoutUtils::ComputeWidthValue(aRenderingContext, aFrame,
                                             0, 0, 0, minWidth);
        if (w > minCoord)
            minCoord = w;
        if (w > prefCoord)
            prefCoord = w;
    } else if (unit == eStyleUnit_Percent) {
        float p = stylePos->mMinWidth.GetPercentValue();
        if (p > prefPercent)
            prefPercent = p;
    }
    

    
    if (aIsCell) {
        minCoord += boxSizingToBorderEdge;
        prefCoord = NSCoordSaturatingAdd(prefCoord, boxSizingToBorderEdge);
    }

    return CellWidthInfo(minCoord, prefCoord, prefPercent, hasSpecifiedWidth);
}

static inline CellWidthInfo
GetCellWidthInfo(nsRenderingContext *aRenderingContext,
                 nsTableCellFrame *aCellFrame)
{
    return GetWidthInfo(aRenderingContext, aCellFrame, true);
}

static inline CellWidthInfo
GetColWidthInfo(nsRenderingContext *aRenderingContext,
                nsIFrame *aFrame)
{
    return GetWidthInfo(aRenderingContext, aFrame, false);
}








void
BasicTableLayoutStrategy::ComputeColumnIntrinsicISizes(nsRenderingContext* aRenderingContext)
{
    nsTableFrame *tableFrame = mTableFrame;
    nsTableCellMap *cellMap = tableFrame->GetCellMap();

    mozilla::AutoStackArena arena;
    SpanningCellSorter spanningCells;

    
    
    int32_t col, col_end;
    for (col = 0, col_end = cellMap->GetColCount(); col < col_end; ++col) {
        nsTableColFrame *colFrame = tableFrame->GetColFrame(col);
        if (!colFrame) {
            NS_ERROR("column frames out of sync with cell map");
            continue;
        }
        colFrame->ResetIntrinsics();
        colFrame->ResetSpanIntrinsics();

        
        CellWidthInfo colInfo = GetColWidthInfo(aRenderingContext, colFrame);
        colFrame->AddCoords(colInfo.minCoord, colInfo.prefCoord,
                            colInfo.hasSpecifiedWidth);
        colFrame->AddPrefPercent(colInfo.prefPercent);

        
        
        

        
        if (colInfo.minCoord == 0 && colInfo.prefCoord == 0 &&
            colInfo.prefPercent == 0.0f) {
            NS_ASSERTION(colFrame->GetParent()->GetType() ==
                             nsGkAtoms::tableColGroupFrame,
                         "expected a column-group");
            colInfo = GetColWidthInfo(aRenderingContext, colFrame->GetParent());
            colFrame->AddCoords(colInfo.minCoord, colInfo.prefCoord,
                                colInfo.hasSpecifiedWidth);
            colFrame->AddPrefPercent(colInfo.prefPercent);
        }

        
        
        nsCellMapColumnIterator columnIter(cellMap, col);
        int32_t row, colSpan;
        nsTableCellFrame* cellFrame;
        while ((cellFrame = columnIter.GetNextFrame(&row, &colSpan))) {
            if (colSpan > 1) {
                spanningCells.AddCell(colSpan, row, col);
                continue;
            }

            CellWidthInfo info = GetCellWidthInfo(aRenderingContext, cellFrame);

            colFrame->AddCoords(info.minCoord, info.prefCoord,
                                info.hasSpecifiedWidth);
            colFrame->AddPrefPercent(info.prefPercent);
        }
#ifdef DEBUG_dbaron_off
        printf("table %p col %d nonspan: min=%d pref=%d spec=%d pct=%f\n",
               mTableFrame, col, colFrame->GetMinCoord(),
               colFrame->GetPrefCoord(), colFrame->GetHasSpecifiedCoord(),
               colFrame->GetPrefPercent());
#endif
    }
#ifdef DEBUG_TABLE_STRATEGY
    printf("ComputeColumnIntrinsicISizes single\n");
    mTableFrame->Dump(false, true, false);
#endif

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    SpanningCellSorter::Item *item;
    int32_t colSpan;
    while ((item = spanningCells.GetNext(&colSpan))) {
        NS_ASSERTION(colSpan > 1,
                     "cell should not have been put in spanning cell sorter");
        do {
            int32_t row = item->row;
            col = item->col;
            CellData *cellData = cellMap->GetDataAt(row, col);
            NS_ASSERTION(cellData && cellData->IsOrig(),
                         "bogus result from spanning cell sorter");

            nsTableCellFrame *cellFrame = cellData->GetCellFrame();
            NS_ASSERTION(cellFrame, "bogus result from spanning cell sorter");

            CellWidthInfo info = GetCellWidthInfo(aRenderingContext, cellFrame);

            if (info.prefPercent > 0.0f) {
                DistributePctWidthToColumns(info.prefPercent,
                                            col, colSpan);
            }
            DistributeWidthToColumns(info.minCoord, col, colSpan, 
                                     BTLS_MIN_WIDTH, info.hasSpecifiedWidth);
            DistributeWidthToColumns(info.prefCoord, col, colSpan, 
                                     BTLS_PREF_WIDTH, info.hasSpecifiedWidth);
        } while ((item = item->next));

        
        

        for (col = 0, col_end = cellMap->GetColCount(); col < col_end; ++col) {
            nsTableColFrame *colFrame = tableFrame->GetColFrame(col);
            if (!colFrame) {
                NS_ERROR("column frames out of sync with cell map");
                continue;
            }

            colFrame->AccumulateSpanIntrinsics();
            colFrame->ResetSpanIntrinsics();

#ifdef DEBUG_dbaron_off
            printf("table %p col %d span %d: min=%d pref=%d spec=%d pct=%f\n",
                   mTableFrame, col, colSpan, colFrame->GetMinCoord(),
                   colFrame->GetPrefCoord(), colFrame->GetHasSpecifiedCoord(),
                   colFrame->GetPrefPercent());
#endif
        }
    }

    
    
    
    
    
    float pct_used = 0.0f;
    for (col = 0, col_end = cellMap->GetColCount(); col < col_end; ++col) {
        nsTableColFrame *colFrame = tableFrame->GetColFrame(col);
        if (!colFrame) {
            NS_ERROR("column frames out of sync with cell map");
            continue;
        }

        colFrame->AdjustPrefPercent(&pct_used);
    }

#ifdef DEBUG_TABLE_STRATEGY
    printf("ComputeColumnIntrinsicISizes spanning\n");
    mTableFrame->Dump(false, true, false);
#endif
}

void
BasicTableLayoutStrategy::ComputeIntrinsicISizes(nsRenderingContext* aRenderingContext)
{
    ComputeColumnIntrinsicISizes(aRenderingContext);

    nsTableCellMap *cellMap = mTableFrame->GetCellMap();
    nscoord min = 0, pref = 0, max_small_pct_pref = 0, nonpct_pref_total = 0;
    float pct_total = 0.0f; 
    int32_t colCount = cellMap->GetColCount();
    
    
    nscoord add = mTableFrame->GetColSpacing(colCount);

    for (int32_t col = 0; col < colCount; ++col) {
        nsTableColFrame *colFrame = mTableFrame->GetColFrame(col);
        if (!colFrame) {
            NS_ERROR("column frames out of sync with cell map");
            continue;
        }
        if (mTableFrame->ColumnHasCellSpacingBefore(col)) {
            add += mTableFrame->GetColSpacing(col - 1);
        }
        min += colFrame->GetMinCoord();
        pref = NSCoordSaturatingAdd(pref, colFrame->GetPrefCoord());

        
        
        float p = colFrame->GetPrefPercent();
        if (p > 0.0f) {
            nscoord colPref = colFrame->GetPrefCoord();
            nscoord new_small_pct_expand = 
                (colPref == nscoord_MAX ?
                 nscoord_MAX : nscoord(float(colPref) / p));
            if (new_small_pct_expand > max_small_pct_pref) {
                max_small_pct_pref = new_small_pct_expand;
            }
            pct_total += p;
        } else {
            nonpct_pref_total = NSCoordSaturatingAdd(nonpct_pref_total, 
                                                     colFrame->GetPrefCoord());
        }
    }

    nscoord pref_pct_expand = pref;

    
    
    if (max_small_pct_pref > pref_pct_expand) {
        pref_pct_expand = max_small_pct_pref;
    }

    
    
    
    
    NS_ASSERTION(0.0f <= pct_total && pct_total <= 1.0f,
                 "column percentage widths not adjusted down to 100%");
    if (pct_total == 1.0f) {
        if (nonpct_pref_total > 0) {
            pref_pct_expand = nscoord_MAX;
            
            
        }
    } else {
        nscoord large_pct_pref =
            (nonpct_pref_total == nscoord_MAX ?
             nscoord_MAX :
             nscoord(float(nonpct_pref_total) / (1.0f - pct_total)));
        if (large_pct_pref > pref_pct_expand)
            pref_pct_expand = large_pct_pref;
    }

    
    if (colCount > 0) {
        min += add;
        pref = NSCoordSaturatingAdd(pref, add);
        pref_pct_expand = NSCoordSaturatingAdd(pref_pct_expand, add);
    }

    mMinWidth = min;
    mPrefWidth = pref;
    mPrefWidthPctExpand = pref_pct_expand;
}

 void
BasicTableLayoutStrategy::MarkIntrinsicISizesDirty()
{
    mMinWidth = NS_INTRINSIC_WIDTH_UNKNOWN;
    mPrefWidth = NS_INTRINSIC_WIDTH_UNKNOWN;
    mPrefWidthPctExpand = NS_INTRINSIC_WIDTH_UNKNOWN;
    mLastCalcWidth = nscoord_MIN;
}

 void
BasicTableLayoutStrategy::ComputeColumnISizes(const nsHTMLReflowState& aReflowState)
{
    nscoord width = aReflowState.ComputedWidth();

    if (mLastCalcWidth == width)
        return;
    mLastCalcWidth = width;

    NS_ASSERTION((mMinWidth == NS_INTRINSIC_WIDTH_UNKNOWN) ==
                 (mPrefWidth == NS_INTRINSIC_WIDTH_UNKNOWN),
                 "dirtyness out of sync");
    NS_ASSERTION((mMinWidth == NS_INTRINSIC_WIDTH_UNKNOWN) ==
                 (mPrefWidthPctExpand == NS_INTRINSIC_WIDTH_UNKNOWN),
                 "dirtyness out of sync");
    
    if (mMinWidth == NS_INTRINSIC_WIDTH_UNKNOWN)
        ComputeIntrinsicISizes(aReflowState.rendContext);

    nsTableCellMap *cellMap = mTableFrame->GetCellMap();
    int32_t colCount = cellMap->GetColCount();
    if (colCount <= 0)
        return; 

    DistributeWidthToColumns(width, 0, colCount, BTLS_FINAL_WIDTH, false);

#ifdef DEBUG_TABLE_STRATEGY
    printf("ComputeColumnISizes final\n");
    mTableFrame->Dump(false, true, false);
#endif
}

void
BasicTableLayoutStrategy::DistributePctWidthToColumns(float aSpanPrefPct,
                                                      int32_t aFirstCol,
                                                      int32_t aColCount)
{
    
    int32_t nonPctColCount = 0; 
    nscoord nonPctTotalPrefWidth = 0; 
    

    int32_t scol, scol_end;
    nsTableCellMap *cellMap = mTableFrame->GetCellMap();
    for (scol = aFirstCol, scol_end = aFirstCol + aColCount;
         scol < scol_end; ++scol) {
        nsTableColFrame *scolFrame = mTableFrame->GetColFrame(scol);
        if (!scolFrame) {
            NS_ERROR("column frames out of sync with cell map");
            continue;
        }
        float scolPct = scolFrame->GetPrefPercent();
        if (scolPct == 0.0f) {
            nonPctTotalPrefWidth += scolFrame->GetPrefCoord();
            if (cellMap->GetNumCellsOriginatingInCol(scol) > 0) {
                ++nonPctColCount;
            }
        } else {
            aSpanPrefPct -= scolPct;
        }
    }

    if (aSpanPrefPct <= 0.0f || nonPctColCount == 0) {
        
        
        return;
    }

    
    
    const bool spanHasNonPctPref = nonPctTotalPrefWidth > 0; 
    for (scol = aFirstCol, scol_end = aFirstCol + aColCount;
         scol < scol_end; ++scol) {
        nsTableColFrame *scolFrame = mTableFrame->GetColFrame(scol);
        if (!scolFrame) {
            NS_ERROR("column frames out of sync with cell map");
            continue;
        }

        if (scolFrame->GetPrefPercent() == 0.0f) {
            NS_ASSERTION((!spanHasNonPctPref ||
                          nonPctTotalPrefWidth != 0) &&
                         nonPctColCount != 0,
                         "should not be zero if we haven't allocated "
                         "all pref percent");

            float allocatedPct; 
            if (spanHasNonPctPref) {
                
                
                allocatedPct = aSpanPrefPct *
                    (float(scolFrame->GetPrefCoord()) /
                     float(nonPctTotalPrefWidth));
            } else if (cellMap->GetNumCellsOriginatingInCol(scol) > 0) {
                
                allocatedPct = aSpanPrefPct / float(nonPctColCount);
            } else {
                allocatedPct = 0.0f;
            }
            
            scolFrame->AddSpanPrefPercent(allocatedPct);

            
            
            aSpanPrefPct -= allocatedPct;
            nonPctTotalPrefWidth -= scolFrame->GetPrefCoord();
            if (cellMap->GetNumCellsOriginatingInCol(scol) > 0) {
                --nonPctColCount;
            }

            if (!aSpanPrefPct) {
                
                NS_ASSERTION(spanHasNonPctPref ? 
                             nonPctTotalPrefWidth == 0 :
                             nonPctColCount == 0,
                             "No more pct width to distribute, but there are "
                             "still cols that need some.");
                return;
            }
        }
    }
}

void
BasicTableLayoutStrategy::DistributeWidthToColumns(nscoord aWidth, 
                                                   int32_t aFirstCol, 
                                                   int32_t aColCount,
                                                   BtlsWidthType aWidthType,
                                                   bool aSpanHasSpecifiedWidth)
{
    NS_ASSERTION(aWidthType != BTLS_FINAL_WIDTH || 
                 (aFirstCol == 0 && 
                  aColCount == mTableFrame->GetCellMap()->GetColCount()),
            "Computing final column widths, but didn't get full column range");


    nscoord subtract = 0;
    
    
    
    for (int32_t col = aFirstCol + 1; col < aFirstCol + aColCount; ++col) {
        if (mTableFrame->ColumnHasCellSpacingBefore(col)) {
            
            subtract += mTableFrame->GetColSpacing(col - 1);
        }
    }
    if (aWidthType == BTLS_FINAL_WIDTH) {
        
        
        
        subtract += (mTableFrame->GetColSpacing(-1) +
                     mTableFrame->GetColSpacing(aColCount));
    }
    aWidth = NSCoordSaturatingSubtract(aWidth, subtract, nscoord_MAX);

    

















































    
    

    nscoord guess_min = 0,
            guess_min_pct = 0,
            guess_min_spec = 0,
            guess_pref = 0,
            total_flex_pref = 0,
            total_fixed_pref = 0;
    float total_pct = 0.0f; 
    int32_t numInfiniteWidthCols = 0;
    int32_t numNonSpecZeroWidthCols = 0;

    int32_t col;
    nsTableCellMap *cellMap = mTableFrame->GetCellMap();
    for (col = aFirstCol; col < aFirstCol + aColCount; ++col) {
        nsTableColFrame *colFrame = mTableFrame->GetColFrame(col);
        if (!colFrame) {
            NS_ERROR("column frames out of sync with cell map");
            continue;
        }
        nscoord min_width = colFrame->GetMinCoord();
        guess_min += min_width;
        if (colFrame->GetPrefPercent() != 0.0f) {
            float pct = colFrame->GetPrefPercent();
            total_pct += pct;
            nscoord val = nscoord(float(aWidth) * pct);
            if (val < min_width)
                val = min_width;
            guess_min_pct += val;
            guess_pref = NSCoordSaturatingAdd(guess_pref, val);
        } else {
            nscoord pref_width = colFrame->GetPrefCoord();
            if (pref_width == nscoord_MAX) {
                ++numInfiniteWidthCols;
            }
            guess_pref = NSCoordSaturatingAdd(guess_pref, pref_width);
            guess_min_pct += min_width;
            if (colFrame->GetHasSpecifiedCoord()) {
                
                
                nscoord delta = NSCoordSaturatingSubtract(pref_width, 
                                                          min_width, 0);
                guess_min_spec = NSCoordSaturatingAdd(guess_min_spec, delta);
                total_fixed_pref = NSCoordSaturatingAdd(total_fixed_pref, 
                                                        pref_width);
            } else if (pref_width == 0) {
                if (cellMap->GetNumCellsOriginatingInCol(col) > 0) {
                    ++numNonSpecZeroWidthCols;
                }
            } else {
                total_flex_pref = NSCoordSaturatingAdd(total_flex_pref,
                                                       pref_width);
            }
        }
    }
    guess_min_spec = NSCoordSaturatingAdd(guess_min_spec, guess_min_pct);

    
    enum Loop2Type {
        FLEX_PCT_SMALL, 
        FLEX_FIXED_SMALL, 
        FLEX_FLEX_SMALL, 
        FLEX_FLEX_LARGE, 
        FLEX_FLEX_LARGE_ZERO, 
        FLEX_FIXED_LARGE, 
        FLEX_PCT_LARGE, 
        FLEX_ALL_LARGE 
    };

    Loop2Type l2t;
    
    
    
    nscoord space; 
    union {
        nscoord c;
        float f;
    } basis; 
    if (aWidth < guess_pref) {
        if (aWidthType != BTLS_FINAL_WIDTH && aWidth <= guess_min) {
            
            return;
        }
        NS_ASSERTION(!(aWidthType == BTLS_FINAL_WIDTH && aWidth < guess_min),
                     "Table width is less than the "
                     "sum of its columns' min widths");
        if (aWidth < guess_min_pct) {
            l2t = FLEX_PCT_SMALL;
            space = aWidth - guess_min;
            basis.c = guess_min_pct - guess_min;
        } else if (aWidth < guess_min_spec) {
            l2t = FLEX_FIXED_SMALL;
            space = aWidth - guess_min_pct;
            basis.c = NSCoordSaturatingSubtract(guess_min_spec, guess_min_pct,
                                                nscoord_MAX);
        } else {
            l2t = FLEX_FLEX_SMALL;
            space = aWidth - guess_min_spec;
            basis.c = NSCoordSaturatingSubtract(guess_pref, guess_min_spec,
                                                nscoord_MAX);
        }
    } else {
        space = NSCoordSaturatingSubtract(aWidth, guess_pref, nscoord_MAX);
        if (total_flex_pref > 0) {
            l2t = FLEX_FLEX_LARGE;
            basis.c = total_flex_pref;
        } else if (numNonSpecZeroWidthCols > 0) {
            l2t = FLEX_FLEX_LARGE_ZERO;
            basis.c = numNonSpecZeroWidthCols;
        } else if (total_fixed_pref > 0) {
            l2t = FLEX_FIXED_LARGE;
            basis.c = total_fixed_pref;
        } else if (total_pct > 0.0f) {
            l2t = FLEX_PCT_LARGE;
            basis.f = total_pct;
        } else {
            l2t = FLEX_ALL_LARGE;
            basis.c = aColCount;
        }
    }

#ifdef DEBUG_dbaron_off
    printf("ComputeColumnISizes: %d columns in width %d,\n"
           "  guesses=[%d,%d,%d,%d], totals=[%d,%d,%f],\n"
           "  l2t=%d, space=%d, basis.c=%d\n",
           aColCount, aWidth,
           guess_min, guess_min_pct, guess_min_spec, guess_pref,
           total_flex_pref, total_fixed_pref, total_pct,
           l2t, space, basis.c);
#endif

    for (col = aFirstCol; col < aFirstCol + aColCount; ++col) {
        nsTableColFrame *colFrame = mTableFrame->GetColFrame(col);
        if (!colFrame) {
            NS_ERROR("column frames out of sync with cell map");
            continue;
        }
        nscoord col_width;

        float pct = colFrame->GetPrefPercent();
        if (pct != 0.0f) {
            col_width = nscoord(float(aWidth) * pct);
            nscoord col_min = colFrame->GetMinCoord();
            if (col_width < col_min)
                col_width = col_min;
        } else {
            col_width = colFrame->GetPrefCoord();
        }

        nscoord col_width_before_adjust = col_width;

        switch (l2t) {
            case FLEX_PCT_SMALL:
                col_width = col_width_before_adjust = colFrame->GetMinCoord();
                if (pct != 0.0f) {
                    nscoord pct_minus_min =
                        nscoord(float(aWidth) * pct) - col_width;
                    if (pct_minus_min > 0) {
                        float c = float(space) / float(basis.c);
                        basis.c -= pct_minus_min;
                        col_width += NSToCoordRound(float(pct_minus_min) * c);
                    }
                }
                break;
            case FLEX_FIXED_SMALL:
                if (pct == 0.0f) {
                    NS_ASSERTION(col_width == colFrame->GetPrefCoord(),
                                 "wrong width assigned");
                    if (colFrame->GetHasSpecifiedCoord()) {
                        nscoord col_min = colFrame->GetMinCoord();
                        nscoord pref_minus_min = col_width - col_min;
                        col_width = col_width_before_adjust = col_min;
                        if (pref_minus_min != 0) {
                            float c = float(space) / float(basis.c);
                            basis.c -= pref_minus_min;
                            col_width += NSToCoordRound(
                                float(pref_minus_min) * c);
                        }
                    } else
                        col_width = col_width_before_adjust =
                            colFrame->GetMinCoord();
                }
                break;
            case FLEX_FLEX_SMALL:
                if (pct == 0.0f &&
                    !colFrame->GetHasSpecifiedCoord()) {
                    NS_ASSERTION(col_width == colFrame->GetPrefCoord(),
                                 "wrong width assigned");
                    nscoord col_min = colFrame->GetMinCoord();
                    nscoord pref_minus_min = 
                        NSCoordSaturatingSubtract(col_width, col_min, 0);
                    col_width = col_width_before_adjust = col_min;
                    if (pref_minus_min != 0) {
                        float c = float(space) / float(basis.c);
                        
                        
                        
                        
                        
                        
                        
                        
                        if (numInfiniteWidthCols) {
                            if (colFrame->GetPrefCoord() == nscoord_MAX) {
                                c = c / float(numInfiniteWidthCols);
                                --numInfiniteWidthCols;
                            } else {
                                c = 0.0f;
                            }
                        }
                        basis.c = NSCoordSaturatingSubtract(basis.c, 
                                                            pref_minus_min,
                                                            nscoord_MAX);
                        col_width += NSToCoordRound(
                            float(pref_minus_min) * c);
                    }
                }
                break;
            case FLEX_FLEX_LARGE:
                if (pct == 0.0f &&
                    !colFrame->GetHasSpecifiedCoord()) {
                    NS_ASSERTION(col_width == colFrame->GetPrefCoord(),
                                 "wrong width assigned");
                    if (col_width != 0) {
                        if (space == nscoord_MAX) {
                            basis.c -= col_width;
                            col_width = nscoord_MAX;
                        } else {
                            float c = float(space) / float(basis.c);
                            basis.c -= col_width;
                            col_width += NSToCoordRound(float(col_width) * c);
                        }
                    }
                }
                break;
            case FLEX_FLEX_LARGE_ZERO:
                if (pct == 0.0f &&
                    !colFrame->GetHasSpecifiedCoord() &&
                    cellMap->GetNumCellsOriginatingInCol(col) > 0) {

                    NS_ASSERTION(col_width == 0 &&
                                 colFrame->GetPrefCoord() == 0,
                                 "Since we're in FLEX_FLEX_LARGE_ZERO case, "
                                 "all auto-width cols should have zero pref "
                                 "width.");
                    float c = float(space) / float(basis.c);
                    col_width += NSToCoordRound(c);
                    --basis.c;
                }
                break;
            case FLEX_FIXED_LARGE:
                if (pct == 0.0f) {
                    NS_ASSERTION(col_width == colFrame->GetPrefCoord(),
                                 "wrong width assigned");
                    NS_ASSERTION(colFrame->GetHasSpecifiedCoord() ||
                                 colFrame->GetPrefCoord() == 0,
                                 "wrong case");
                    if (col_width != 0) {
                        float c = float(space) / float(basis.c);
                        basis.c -= col_width;
                        col_width += NSToCoordRound(float(col_width) * c);
                    }
                }
                break;
            case FLEX_PCT_LARGE:
                NS_ASSERTION(pct != 0.0f || colFrame->GetPrefCoord() == 0,
                             "wrong case");
                if (pct != 0.0f) {
                    float c = float(space) / basis.f;
                    col_width += NSToCoordRound(pct * c);
                    basis.f -= pct;
                }
                break;
            case FLEX_ALL_LARGE:
                {
                    float c = float(space) / float(basis.c);
                    col_width += NSToCoordRound(c);
                    --basis.c;
                }
                break;
        }

        
        if (space != nscoord_MAX) {
            NS_ASSERTION(col_width != nscoord_MAX,
                 "How is col_width nscoord_MAX if space isn't?");
            NS_ASSERTION(col_width_before_adjust != nscoord_MAX,
                 "How is col_width_before_adjust nscoord_MAX if space isn't?");
            space -= col_width - col_width_before_adjust;
        }

        NS_ASSERTION(col_width >= colFrame->GetMinCoord(),
                     "assigned width smaller than min");
        
        
        switch (aWidthType) {
            case BTLS_MIN_WIDTH:
                {
                    
                    
                    
                    
                    colFrame->AddSpanCoords(col_width, col_width, 
                                            aSpanHasSpecifiedWidth);
                }
                break;
            case BTLS_PREF_WIDTH:
                {
                    
                    
                    
                    colFrame->AddSpanCoords(0, col_width, 
                                            aSpanHasSpecifiedWidth);
                }
                break;
            case BTLS_FINAL_WIDTH:
                {
                    nscoord old_final = colFrame->GetFinalISize();
                    colFrame->SetFinalISize(col_width);
                    
                    if (old_final != col_width)
                        mTableFrame->DidResizeColumns();
                }
                break;                
        }
    }
    NS_ASSERTION((space == 0 || space == nscoord_MAX) &&
                 ((l2t == FLEX_PCT_LARGE)
                    ? (-0.001f < basis.f && basis.f < 0.001f)
                    : (basis.c == 0 || basis.c == nscoord_MAX)),
                 "didn't subtract all that we added");
}
