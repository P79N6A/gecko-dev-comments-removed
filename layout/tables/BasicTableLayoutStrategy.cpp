










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
    DISPLAY_MIN_WIDTH(mTableFrame, mMinISize);
    if (mMinISize == NS_INTRINSIC_WIDTH_UNKNOWN) {
        ComputeIntrinsicISizes(aRenderingContext);
    }
    return mMinISize;
}

 nscoord
BasicTableLayoutStrategy::GetPrefISize(nsRenderingContext* aRenderingContext,
                                       bool aComputingSize)
{
    DISPLAY_PREF_WIDTH(mTableFrame, mPrefISize);
    NS_ASSERTION((mPrefISize == NS_INTRINSIC_WIDTH_UNKNOWN) ==
                 (mPrefISizePctExpand == NS_INTRINSIC_WIDTH_UNKNOWN),
                 "dirtyness out of sync");
    if (mPrefISize == NS_INTRINSIC_WIDTH_UNKNOWN) {
        ComputeIntrinsicISizes(aRenderingContext);
    }
    return aComputingSize ? mPrefISizePctExpand : mPrefISize;
}

struct CellISizeInfo {
    CellISizeInfo(nscoord aMinCoord, nscoord aPrefCoord,
                  float aPrefPercent, bool aHasSpecifiedISize)
        : hasSpecifiedISize(aHasSpecifiedISize)
        , minCoord(aMinCoord)
        , prefCoord(aPrefCoord)
        , prefPercent(aPrefPercent)
    {
    }

    bool hasSpecifiedISize;
    nscoord minCoord;
    nscoord prefCoord;
    float prefPercent;
};



static CellISizeInfo
GetISizeInfo(nsRenderingContext *aRenderingContext,
             nsIFrame *aFrame, WritingMode aWM, bool aIsCell)
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
        
        
        
        
        
        

        
        nsIFrame::IntrinsicISizeOffsetData offsets =
            aFrame->IntrinsicISizeOffsets();

        
        
        
        
        
        
        
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
    bool hasSpecifiedISize = false;

    const nsStyleCoord& iSize = stylePos->ISize(aWM);
    nsStyleUnit unit = iSize.GetUnit();
    
    
    
    
    if (iSize.ConvertsToLength()) {
        hasSpecifiedISize = true;
        
        
        
        
        nscoord c = nsLayoutUtils::ComputeISizeValue(aRenderingContext,
                                                     aFrame, 0, 0, 0, iSize);
        
        
        
        
        
        if (aIsCell && c > minCoord && isQuirks &&
            aFrame->GetContent()->HasAttr(kNameSpaceID_None,
                                          nsGkAtoms::nowrap)) {
            minCoord = c;
        }
        prefCoord = std::max(c, minCoord);
    } else if (unit == eStyleUnit_Percent) {
        prefPercent = iSize.GetPercentValue();
    } else if (unit == eStyleUnit_Enumerated && aIsCell) {
        switch (iSize.GetIntValue()) {
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

    nsStyleCoord maxISize(stylePos->MaxISize(aWM));
    if (maxISize.GetUnit() == eStyleUnit_Enumerated) {
        if (!aIsCell || maxISize.GetIntValue() == NS_STYLE_WIDTH_AVAILABLE) {
            maxISize.SetNoneValue();
        } else if (maxISize.GetIntValue() == NS_STYLE_WIDTH_FIT_CONTENT) {
            
            
            maxISize.SetIntValue(NS_STYLE_WIDTH_MAX_CONTENT,
                                 eStyleUnit_Enumerated);
        }
    }
    unit = maxISize.GetUnit();
    
    
    if (maxISize.ConvertsToLength() || unit == eStyleUnit_Enumerated) {
        nscoord c =
            nsLayoutUtils::ComputeISizeValue(aRenderingContext, aFrame,
                                             0, 0, 0, maxISize);
        minCoord = std::min(c, minCoord);
        prefCoord = std::min(c, prefCoord);
    } else if (unit == eStyleUnit_Percent) {
        float p = stylePos->MaxISize(aWM).GetPercentValue();
        if (p < prefPercent) {
            prefPercent = p;
        }
    }
    

    nsStyleCoord minISize(stylePos->MinISize(aWM));
    if (minISize.GetUnit() == eStyleUnit_Enumerated) {
        if (!aIsCell || minISize.GetIntValue() == NS_STYLE_WIDTH_AVAILABLE) {
            minISize.SetCoordValue(0);
        } else if (minISize.GetIntValue() == NS_STYLE_WIDTH_FIT_CONTENT) {
            
            
            minISize.SetIntValue(NS_STYLE_WIDTH_MIN_CONTENT,
                                 eStyleUnit_Enumerated);
        }
    }
    unit = minISize.GetUnit();
    if (minISize.ConvertsToLength() || unit == eStyleUnit_Enumerated) {
        nscoord c =
            nsLayoutUtils::ComputeISizeValue(aRenderingContext, aFrame,
                                             0, 0, 0, minISize);
        minCoord = std::max(c, minCoord);
        prefCoord = std::max(c, prefCoord);
    } else if (unit == eStyleUnit_Percent) {
        float p = stylePos->MinISize(aWM).GetPercentValue();
        if (p > prefPercent) {
            prefPercent = p;
        }
    }
    

    
    if (aIsCell) {
        minCoord += boxSizingToBorderEdge;
        prefCoord = NSCoordSaturatingAdd(prefCoord, boxSizingToBorderEdge);
    }

    return CellISizeInfo(minCoord, prefCoord, prefPercent, hasSpecifiedISize);
}

static inline CellISizeInfo
GetCellISizeInfo(nsRenderingContext *aRenderingContext,
                 nsTableCellFrame *aCellFrame, WritingMode aWM)
{
    return GetISizeInfo(aRenderingContext, aCellFrame, aWM, true);
}

static inline CellISizeInfo
GetColISizeInfo(nsRenderingContext *aRenderingContext,
                nsIFrame *aFrame, WritingMode aWM)
{
    return GetISizeInfo(aRenderingContext, aFrame, aWM, false);
}








void
BasicTableLayoutStrategy::ComputeColumnIntrinsicISizes(nsRenderingContext* aRenderingContext)
{
    nsTableFrame *tableFrame = mTableFrame;
    nsTableCellMap *cellMap = tableFrame->GetCellMap();
    WritingMode wm = tableFrame->GetWritingMode();

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

        
        CellISizeInfo colInfo = GetColISizeInfo(aRenderingContext,
                                                colFrame, wm);
        colFrame->AddCoords(colInfo.minCoord, colInfo.prefCoord,
                            colInfo.hasSpecifiedISize);
        colFrame->AddPrefPercent(colInfo.prefPercent);

        
        
        

        
        if (colInfo.minCoord == 0 && colInfo.prefCoord == 0 &&
            colInfo.prefPercent == 0.0f) {
            NS_ASSERTION(colFrame->GetParent()->GetType() ==
                             nsGkAtoms::tableColGroupFrame,
                         "expected a column-group");
            colInfo = GetColISizeInfo(aRenderingContext,
                                      colFrame->GetParent(), wm);
            colFrame->AddCoords(colInfo.minCoord, colInfo.prefCoord,
                                colInfo.hasSpecifiedISize);
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

            CellISizeInfo info = GetCellISizeInfo(aRenderingContext,
                                                  cellFrame, wm);

            colFrame->AddCoords(info.minCoord, info.prefCoord,
                                info.hasSpecifiedISize);
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

            CellISizeInfo info =
                GetCellISizeInfo(aRenderingContext, cellFrame, wm);

            if (info.prefPercent > 0.0f) {
                DistributePctISizeToColumns(info.prefPercent,
                                            col, colSpan);
            }
            DistributeISizeToColumns(info.minCoord, col, colSpan, 
                                     BTLS_MIN_ISIZE, info.hasSpecifiedISize);
            DistributeISizeToColumns(info.prefCoord, col, colSpan, 
                                     BTLS_PREF_ISIZE, info.hasSpecifiedISize);
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
                 "column percentage inline-sizes not adjusted down to 100%");
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

    mMinISize = min;
    mPrefISize = pref;
    mPrefISizePctExpand = pref_pct_expand;
}

 void
BasicTableLayoutStrategy::MarkIntrinsicISizesDirty()
{
    mMinISize = NS_INTRINSIC_WIDTH_UNKNOWN;
    mPrefISize = NS_INTRINSIC_WIDTH_UNKNOWN;
    mPrefISizePctExpand = NS_INTRINSIC_WIDTH_UNKNOWN;
    mLastCalcISize = nscoord_MIN;
}

 void
BasicTableLayoutStrategy::ComputeColumnISizes(const nsHTMLReflowState& aReflowState)
{
    nscoord iSize = aReflowState.ComputedISize();

    if (mLastCalcISize == iSize) {
        return;
    }
    mLastCalcISize = iSize;

    NS_ASSERTION((mMinISize == NS_INTRINSIC_WIDTH_UNKNOWN) ==
                 (mPrefISize == NS_INTRINSIC_WIDTH_UNKNOWN),
                 "dirtyness out of sync");
    NS_ASSERTION((mMinISize == NS_INTRINSIC_WIDTH_UNKNOWN) ==
                 (mPrefISizePctExpand == NS_INTRINSIC_WIDTH_UNKNOWN),
                 "dirtyness out of sync");
    
    if (mMinISize == NS_INTRINSIC_WIDTH_UNKNOWN) {
        ComputeIntrinsicISizes(aReflowState.rendContext);
    }

    nsTableCellMap *cellMap = mTableFrame->GetCellMap();
    int32_t colCount = cellMap->GetColCount();
    if (colCount <= 0)
        return; 

    DistributeISizeToColumns(iSize, 0, colCount, BTLS_FINAL_ISIZE, false);

#ifdef DEBUG_TABLE_STRATEGY
    printf("ComputeColumnISizes final\n");
    mTableFrame->Dump(false, true, false);
#endif
}

void
BasicTableLayoutStrategy::DistributePctISizeToColumns(float aSpanPrefPct,
                                                      int32_t aFirstCol,
                                                      int32_t aColCount)
{
    
    int32_t nonPctColCount = 0; 
    nscoord nonPctTotalPrefISize = 0; 
    

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
            nonPctTotalPrefISize += scolFrame->GetPrefCoord();
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

    
    
    const bool spanHasNonPctPref = nonPctTotalPrefISize > 0; 
    for (scol = aFirstCol, scol_end = aFirstCol + aColCount;
         scol < scol_end; ++scol) {
        nsTableColFrame *scolFrame = mTableFrame->GetColFrame(scol);
        if (!scolFrame) {
            NS_ERROR("column frames out of sync with cell map");
            continue;
        }

        if (scolFrame->GetPrefPercent() == 0.0f) {
            NS_ASSERTION((!spanHasNonPctPref ||
                          nonPctTotalPrefISize != 0) &&
                         nonPctColCount != 0,
                         "should not be zero if we haven't allocated "
                         "all pref percent");

            float allocatedPct; 
            if (spanHasNonPctPref) {
                
                
                allocatedPct = aSpanPrefPct *
                    (float(scolFrame->GetPrefCoord()) /
                     float(nonPctTotalPrefISize));
            } else if (cellMap->GetNumCellsOriginatingInCol(scol) > 0) {
                
                allocatedPct = aSpanPrefPct / float(nonPctColCount);
            } else {
                allocatedPct = 0.0f;
            }
            
            scolFrame->AddSpanPrefPercent(allocatedPct);

            
            
            aSpanPrefPct -= allocatedPct;
            nonPctTotalPrefISize -= scolFrame->GetPrefCoord();
            if (cellMap->GetNumCellsOriginatingInCol(scol) > 0) {
                --nonPctColCount;
            }

            if (!aSpanPrefPct) {
                
                NS_ASSERTION(spanHasNonPctPref ? 
                             nonPctTotalPrefISize == 0 :
                             nonPctColCount == 0,
                             "No more pct inline-size to distribute, "
                             "but there are still cols that need some.");
                return;
            }
        }
    }
}

void
BasicTableLayoutStrategy::DistributeISizeToColumns(nscoord aISize,
                                                   int32_t aFirstCol,
                                                   int32_t aColCount,
                                                   BtlsISizeType aISizeType,
                                                   bool aSpanHasSpecifiedISize)
{
    NS_ASSERTION(aISizeType != BTLS_FINAL_ISIZE ||
                 (aFirstCol == 0 && 
                  aColCount == mTableFrame->GetCellMap()->GetColCount()),
            "Computing final column isizes, but didn't get full column range");

    nscoord subtract = 0;
    
    
    
    for (int32_t col = aFirstCol + 1; col < aFirstCol + aColCount; ++col) {
        if (mTableFrame->ColumnHasCellSpacingBefore(col)) {
            
            subtract += mTableFrame->GetColSpacing(col - 1);
        }
    }
    if (aISizeType == BTLS_FINAL_ISIZE) {
        
        
        
        subtract += (mTableFrame->GetColSpacing(-1) +
                     mTableFrame->GetColSpacing(aColCount));
    }
    aISize = NSCoordSaturatingSubtract(aISize, subtract, nscoord_MAX);

    

















































    
    

    nscoord guess_min = 0,
            guess_min_pct = 0,
            guess_min_spec = 0,
            guess_pref = 0,
            total_flex_pref = 0,
            total_fixed_pref = 0;
    float total_pct = 0.0f; 
    int32_t numInfiniteISizeCols = 0;
    int32_t numNonSpecZeroISizeCols = 0;

    int32_t col;
    nsTableCellMap *cellMap = mTableFrame->GetCellMap();
    for (col = aFirstCol; col < aFirstCol + aColCount; ++col) {
        nsTableColFrame *colFrame = mTableFrame->GetColFrame(col);
        if (!colFrame) {
            NS_ERROR("column frames out of sync with cell map");
            continue;
        }
        nscoord min_iSize = colFrame->GetMinCoord();
        guess_min += min_iSize;
        if (colFrame->GetPrefPercent() != 0.0f) {
            float pct = colFrame->GetPrefPercent();
            total_pct += pct;
            nscoord val = nscoord(float(aISize) * pct);
            if (val < min_iSize) {
                val = min_iSize;
            }
            guess_min_pct += val;
            guess_pref = NSCoordSaturatingAdd(guess_pref, val);
        } else {
            nscoord pref_iSize = colFrame->GetPrefCoord();
            if (pref_iSize == nscoord_MAX) {
                ++numInfiniteISizeCols;
            }
            guess_pref = NSCoordSaturatingAdd(guess_pref, pref_iSize);
            guess_min_pct += min_iSize;
            if (colFrame->GetHasSpecifiedCoord()) {
                
                
                nscoord delta = NSCoordSaturatingSubtract(pref_iSize, 
                                                          min_iSize, 0);
                guess_min_spec = NSCoordSaturatingAdd(guess_min_spec, delta);
                total_fixed_pref = NSCoordSaturatingAdd(total_fixed_pref, 
                                                        pref_iSize);
            } else if (pref_iSize == 0) {
                if (cellMap->GetNumCellsOriginatingInCol(col) > 0) {
                    ++numNonSpecZeroISizeCols;
                }
            } else {
                total_flex_pref = NSCoordSaturatingAdd(total_flex_pref,
                                                       pref_iSize);
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
    if (aISize < guess_pref) {
        if (aISizeType != BTLS_FINAL_ISIZE && aISize <= guess_min) {
            
            return;
        }
        NS_ASSERTION(!(aISizeType == BTLS_FINAL_ISIZE && aISize < guess_min),
                     "Table inline-size is less than the "
                     "sum of its columns' min inline-sizes");
        if (aISize < guess_min_pct) {
            l2t = FLEX_PCT_SMALL;
            space = aISize - guess_min;
            basis.c = guess_min_pct - guess_min;
        } else if (aISize < guess_min_spec) {
            l2t = FLEX_FIXED_SMALL;
            space = aISize - guess_min_pct;
            basis.c = NSCoordSaturatingSubtract(guess_min_spec, guess_min_pct,
                                                nscoord_MAX);
        } else {
            l2t = FLEX_FLEX_SMALL;
            space = aISize - guess_min_spec;
            basis.c = NSCoordSaturatingSubtract(guess_pref, guess_min_spec,
                                                nscoord_MAX);
        }
    } else {
        space = NSCoordSaturatingSubtract(aISize, guess_pref, nscoord_MAX);
        if (total_flex_pref > 0) {
            l2t = FLEX_FLEX_LARGE;
            basis.c = total_flex_pref;
        } else if (numNonSpecZeroISizeCols > 0) {
            l2t = FLEX_FLEX_LARGE_ZERO;
            basis.c = numNonSpecZeroISizeCols;
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
    printf("ComputeColumnISizes: %d columns in isize %d,\n"
           "  guesses=[%d,%d,%d,%d], totals=[%d,%d,%f],\n"
           "  l2t=%d, space=%d, basis.c=%d\n",
           aColCount, aISize,
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
        nscoord col_iSize;

        float pct = colFrame->GetPrefPercent();
        if (pct != 0.0f) {
            col_iSize = nscoord(float(aISize) * pct);
            nscoord col_min = colFrame->GetMinCoord();
            if (col_iSize < col_min) {
                col_iSize = col_min;
            }
        } else {
            col_iSize = colFrame->GetPrefCoord();
        }

        nscoord col_iSize_before_adjust = col_iSize;

        switch (l2t) {
            case FLEX_PCT_SMALL:
                col_iSize = col_iSize_before_adjust = colFrame->GetMinCoord();
                if (pct != 0.0f) {
                    nscoord pct_minus_min =
                        nscoord(float(aISize) * pct) - col_iSize;
                    if (pct_minus_min > 0) {
                        float c = float(space) / float(basis.c);
                        basis.c -= pct_minus_min;
                        col_iSize += NSToCoordRound(float(pct_minus_min) * c);
                    }
                }
                break;
            case FLEX_FIXED_SMALL:
                if (pct == 0.0f) {
                    NS_ASSERTION(col_iSize == colFrame->GetPrefCoord(),
                                 "wrong inline-size assigned");
                    if (colFrame->GetHasSpecifiedCoord()) {
                        nscoord col_min = colFrame->GetMinCoord();
                        nscoord pref_minus_min = col_iSize - col_min;
                        col_iSize = col_iSize_before_adjust = col_min;
                        if (pref_minus_min != 0) {
                            float c = float(space) / float(basis.c);
                            basis.c -= pref_minus_min;
                            col_iSize += NSToCoordRound(
                                float(pref_minus_min) * c);
                        }
                    } else
                        col_iSize = col_iSize_before_adjust =
                            colFrame->GetMinCoord();
                }
                break;
            case FLEX_FLEX_SMALL:
                if (pct == 0.0f &&
                    !colFrame->GetHasSpecifiedCoord()) {
                    NS_ASSERTION(col_iSize == colFrame->GetPrefCoord(),
                                 "wrong inline-size assigned");
                    nscoord col_min = colFrame->GetMinCoord();
                    nscoord pref_minus_min = 
                        NSCoordSaturatingSubtract(col_iSize, col_min, 0);
                    col_iSize = col_iSize_before_adjust = col_min;
                    if (pref_minus_min != 0) {
                        float c = float(space) / float(basis.c);
                        
                        
                        
                        
                        
                        
                        
                        
                        if (numInfiniteISizeCols) {
                            if (colFrame->GetPrefCoord() == nscoord_MAX) {
                                c = c / float(numInfiniteISizeCols);
                                --numInfiniteISizeCols;
                            } else {
                                c = 0.0f;
                            }
                        }
                        basis.c = NSCoordSaturatingSubtract(basis.c, 
                                                            pref_minus_min,
                                                            nscoord_MAX);
                        col_iSize += NSToCoordRound(
                            float(pref_minus_min) * c);
                    }
                }
                break;
            case FLEX_FLEX_LARGE:
                if (pct == 0.0f &&
                    !colFrame->GetHasSpecifiedCoord()) {
                    NS_ASSERTION(col_iSize == colFrame->GetPrefCoord(),
                                 "wrong inline-size assigned");
                    if (col_iSize != 0) {
                        if (space == nscoord_MAX) {
                            basis.c -= col_iSize;
                            col_iSize = nscoord_MAX;
                        } else {
                            float c = float(space) / float(basis.c);
                            basis.c -= col_iSize;
                            col_iSize += NSToCoordRound(float(col_iSize) * c);
                        }
                    }
                }
                break;
            case FLEX_FLEX_LARGE_ZERO:
                if (pct == 0.0f &&
                    !colFrame->GetHasSpecifiedCoord() &&
                    cellMap->GetNumCellsOriginatingInCol(col) > 0) {

                    NS_ASSERTION(col_iSize == 0 &&
                                 colFrame->GetPrefCoord() == 0,
                                 "Since we're in FLEX_FLEX_LARGE_ZERO case, "
                                 "all auto-inline-size cols should have zero "
                                 "pref inline-size.");
                    float c = float(space) / float(basis.c);
                    col_iSize += NSToCoordRound(c);
                    --basis.c;
                }
                break;
            case FLEX_FIXED_LARGE:
                if (pct == 0.0f) {
                    NS_ASSERTION(col_iSize == colFrame->GetPrefCoord(),
                                 "wrong inline-size assigned");
                    NS_ASSERTION(colFrame->GetHasSpecifiedCoord() ||
                                 colFrame->GetPrefCoord() == 0,
                                 "wrong case");
                    if (col_iSize != 0) {
                        float c = float(space) / float(basis.c);
                        basis.c -= col_iSize;
                        col_iSize += NSToCoordRound(float(col_iSize) * c);
                    }
                }
                break;
            case FLEX_PCT_LARGE:
                NS_ASSERTION(pct != 0.0f || colFrame->GetPrefCoord() == 0,
                             "wrong case");
                if (pct != 0.0f) {
                    float c = float(space) / basis.f;
                    col_iSize += NSToCoordRound(pct * c);
                    basis.f -= pct;
                }
                break;
            case FLEX_ALL_LARGE:
                {
                    float c = float(space) / float(basis.c);
                    col_iSize += NSToCoordRound(c);
                    --basis.c;
                }
                break;
        }

        
        if (space != nscoord_MAX) {
            NS_ASSERTION(col_iSize != nscoord_MAX,
                 "How is col_iSize nscoord_MAX if space isn't?");
            NS_ASSERTION(col_iSize_before_adjust != nscoord_MAX,
                 "How is col_iSize_before_adjust nscoord_MAX if space isn't?");
            space -= col_iSize - col_iSize_before_adjust;
        }

        NS_ASSERTION(col_iSize >= colFrame->GetMinCoord(),
                     "assigned inline-size smaller than min");
        
        
        switch (aISizeType) {
            case BTLS_MIN_ISIZE:
                {
                    
                    
                    
                    
                    colFrame->AddSpanCoords(col_iSize, col_iSize, 
                                            aSpanHasSpecifiedISize);
                }
                break;
            case BTLS_PREF_ISIZE:
                {
                    
                    
                    
                    colFrame->AddSpanCoords(0, col_iSize, 
                                            aSpanHasSpecifiedISize);
                }
                break;
            case BTLS_FINAL_ISIZE:
                {
                    nscoord old_final = colFrame->GetFinalISize();
                    colFrame->SetFinalISize(col_iSize);
                    
                    if (old_final != col_iSize) {
                        mTableFrame->DidResizeColumns();
                    }
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
