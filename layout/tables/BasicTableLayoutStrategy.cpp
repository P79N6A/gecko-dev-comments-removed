










































#include "BasicTableLayoutStrategy.h"
#include "nsTableFrame.h"
#include "nsTableCellFrame.h"
#include "nsLayoutUtils.h"
#include "nsGkAtoms.h"
#include "SpanningCellSorter.h"

#undef  DEBUG_TABLE_STRATEGY 

BasicTableLayoutStrategy::BasicTableLayoutStrategy(nsTableFrame *aTableFrame)
  : mTableFrame(aTableFrame)
{
    MarkIntrinsicWidthsDirty();
}


BasicTableLayoutStrategy::~BasicTableLayoutStrategy()
{
}

 nscoord
BasicTableLayoutStrategy::GetMinWidth(nsIRenderingContext* aRenderingContext)
{
    DISPLAY_MIN_WIDTH(mTableFrame, mMinWidth);
    if (mMinWidth == NS_INTRINSIC_WIDTH_UNKNOWN)
        ComputeIntrinsicWidths(aRenderingContext);
    return mMinWidth;
}

 nscoord
BasicTableLayoutStrategy::GetPrefWidth(nsIRenderingContext* aRenderingContext,
                                       PRBool aComputingSize)
{
    DISPLAY_PREF_WIDTH(mTableFrame, mPrefWidth);
    NS_ASSERTION((mPrefWidth == NS_INTRINSIC_WIDTH_UNKNOWN) ==
                 (mPrefWidthPctExpand == NS_INTRINSIC_WIDTH_UNKNOWN),
                 "dirtyness out of sync");
    if (mPrefWidth == NS_INTRINSIC_WIDTH_UNKNOWN)
        ComputeIntrinsicWidths(aRenderingContext);
    return aComputingSize ? mPrefWidthPctExpand : mPrefWidth;
}

struct CellWidthInfo {
    CellWidthInfo(nscoord aMinCoord, nscoord aPrefCoord,
                  float aPrefPercent, PRBool aHasSpecifiedWidth)
        : hasSpecifiedWidth(aHasSpecifiedWidth)
        , minCoord(aMinCoord)
        , prefCoord(aPrefCoord)
        , prefPercent(aPrefPercent)
    {
    }

    PRBool hasSpecifiedWidth;
    nscoord minCoord;
    nscoord prefCoord;
    float prefPercent;
};



static CellWidthInfo
GetWidthInfo(nsIRenderingContext *aRenderingContext,
             nsIFrame *aFrame,
             PRBool aIsCell,
             const nsStylePosition *aStylePos)
{
    nscoord minCoord, prefCoord;
    if (aIsCell) {
        minCoord = aFrame->GetMinWidth(aRenderingContext);
        prefCoord = aFrame->GetPrefWidth(aRenderingContext);
    } else {
        minCoord = 0;
        prefCoord = 0;
    }
    float prefPercent = 0.0f;
    PRBool hasSpecifiedWidth = PR_FALSE;

    

    nsStyleUnit unit = aStylePos->mWidth.GetUnit();
    if (unit == eStyleUnit_Coord || unit == eStyleUnit_Chars) {
        hasSpecifiedWidth = PR_TRUE;
        nscoord w = nsLayoutUtils::ComputeWidthValue(aRenderingContext,
                      aFrame, 0, 0, 0, aStylePos->mWidth);
        
        
        
        
        
        if (aIsCell && w > minCoord &&
            aFrame->PresContext()->CompatibilityMode() ==
              eCompatibility_NavQuirks &&
            aFrame->GetContent()->HasAttr(kNameSpaceID_None,
                                          nsGkAtoms::nowrap)) {
            minCoord = w;
        }
        prefCoord = PR_MAX(w, minCoord);
    } else if (unit == eStyleUnit_Percent) {
        prefPercent = aStylePos->mWidth.GetPercentValue();
    } else if (unit == eStyleUnit_Enumerated && aIsCell) {
        switch (aStylePos->mWidth.GetIntValue()) {
            case NS_STYLE_WIDTH_INTRINSIC:
                
                
                break;
            case NS_STYLE_WIDTH_MIN_INTRINSIC:
                prefCoord = minCoord;
                break;
            case NS_STYLE_WIDTH_SHRINK_WRAP:
            case NS_STYLE_WIDTH_FILL:
                
                break;
            default:
                NS_NOTREACHED("unexpected enumerated value");
        }
    }

    nsStyleCoord maxWidth(aStylePos->mMaxWidth);
    if (maxWidth.GetUnit() == eStyleUnit_Enumerated) {
        if (!aIsCell || maxWidth.GetIntValue() == NS_STYLE_WIDTH_FILL)
            maxWidth.SetNoneValue();
        else if (maxWidth.GetIntValue() == NS_STYLE_WIDTH_SHRINK_WRAP)
            
            
            maxWidth.SetIntValue(NS_STYLE_WIDTH_INTRINSIC,
                                 eStyleUnit_Enumerated);
    }
    unit = maxWidth.GetUnit();
    
    
    if (unit == eStyleUnit_Coord || unit == eStyleUnit_Chars ||
        unit == eStyleUnit_Enumerated) {
        nscoord w =
            nsLayoutUtils::ComputeWidthValue(aRenderingContext, aFrame,
                                             0, 0, 0, maxWidth);
        if (w < minCoord)
            minCoord = w;
        if (w < prefCoord)
            prefCoord = w;
    } else if (unit == eStyleUnit_Percent) {
        float p = aStylePos->mMaxWidth.GetPercentValue();
        if (p < prefPercent)
            prefPercent = p;
    }

    nsStyleCoord minWidth(aStylePos->mMinWidth);
    if (minWidth.GetUnit() == eStyleUnit_Enumerated) {
        if (!aIsCell || minWidth.GetIntValue() == NS_STYLE_WIDTH_FILL)
            minWidth.SetCoordValue(0);
        else if (minWidth.GetIntValue() == NS_STYLE_WIDTH_SHRINK_WRAP)
            
            
            minWidth.SetIntValue(NS_STYLE_WIDTH_MIN_INTRINSIC,
                                 eStyleUnit_Enumerated);
    }
    unit = minWidth.GetUnit();
    if (unit == eStyleUnit_Coord || unit == eStyleUnit_Chars ||
        unit == eStyleUnit_Enumerated) {
        nscoord w =
            nsLayoutUtils::ComputeWidthValue(aRenderingContext, aFrame,
                                             0, 0, 0, minWidth);
        if (w > minCoord)
            minCoord = w;
        if (w > prefCoord)
            prefCoord = w;
    } else if (unit == eStyleUnit_Percent) {
        float p = aStylePos->mMinWidth.GetPercentValue();
        if (p > prefPercent)
            prefPercent = p;
    }

    
    if (aIsCell) {
        nsIFrame::IntrinsicWidthOffsetData offsets =
            aFrame->IntrinsicWidthOffsets(aRenderingContext);
        
        nscoord add = offsets.hPadding + offsets.hBorder;
        minCoord += add;
        prefCoord += add;
    }

    return CellWidthInfo(minCoord, prefCoord, prefPercent, hasSpecifiedWidth);
}

static inline CellWidthInfo
GetCellWidthInfo(nsIRenderingContext *aRenderingContext,
                 nsTableCellFrame *aCellFrame)
{
    return GetWidthInfo(aRenderingContext, aCellFrame, PR_TRUE,
                        aCellFrame->GetStylePosition());
}

static inline CellWidthInfo
GetColWidthInfo(nsIRenderingContext *aRenderingContext,
                nsIFrame *aFrame)
{
    return GetWidthInfo(aRenderingContext, aFrame, PR_FALSE,
                        aFrame->GetStylePosition());
}








void
BasicTableLayoutStrategy::ComputeColumnIntrinsicWidths(nsIRenderingContext* aRenderingContext)
{
    nsTableFrame *tableFrame = mTableFrame;
    nsTableCellMap *cellMap = tableFrame->GetCellMap();

    nscoord spacing = tableFrame->GetCellSpacingX();
    SpanningCellSorter spanningCells(tableFrame->PresContext()->PresShell());

    
    
    PRInt32 col, col_end;
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

        
        
        
        
        NS_ASSERTION(colFrame->GetParent()->GetType() ==
                         nsGkAtoms::tableColGroupFrame,
                     "expected a column-group");
        colInfo = GetColWidthInfo(aRenderingContext, colFrame->GetParent());
        colFrame->AddCoords(colInfo.minCoord, colInfo.prefCoord,
                            colInfo.hasSpecifiedWidth);
        colFrame->AddPrefPercent(colInfo.prefPercent);

        
        
        nsCellMapColumnIterator columnIter(cellMap, col);
        PRInt32 row, colSpan;
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
    printf("ComputeColumnIntrinsicWidths single\n");
    mTableFrame->Dump(PR_FALSE, PR_TRUE, PR_FALSE);
#endif

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    SpanningCellSorter::Item *item;
    PRInt32 colSpan;
    while ((item = spanningCells.GetNext(&colSpan))) {
        NS_ASSERTION(colSpan > 1,
                     "cell should not have been put in spanning cell sorter");
        do {
            PRInt32 row = item->row;
            col = item->col;
            CellData *cellData = cellMap->GetDataAt(row, col);
            NS_ASSERTION(cellData && cellData->IsOrig(),
                         "bogus result from spanning cell sorter");

            nsTableCellFrame *cellFrame = cellData->GetCellFrame();
            NS_ASSERTION(cellFrame, "bogus result from spanning cell sorter");

            CellWidthInfo info = GetCellWidthInfo(aRenderingContext, cellFrame);

            
            
            
            

            
            
            nscoord totalSPref = 0, totalSMin = 0; 
            nscoord totalSNonPctPref = 0; 
                                          
            nscoord totalSAutoPref = 0; 
            PRInt32 nonPctCount = 0; 
            PRInt32 scol, scol_end;
            for (scol = col, scol_end = col + colSpan;
                 scol < scol_end; ++scol) {
                nsTableColFrame *scolFrame = tableFrame->GetColFrame(scol);
                if (!scolFrame) {
                    NS_ERROR("column frames out of sync with cell map");
                    continue;
                }

                if (mTableFrame->GetNumCellsOriginatingInCol(scol) &&
                    scol != col) {
                    info.minCoord -= spacing;
                    info.prefCoord -= spacing;
                }

                totalSPref += scolFrame->GetPrefCoord();
                totalSMin += scolFrame->GetMinCoord();
                if (!scolFrame->GetHasSpecifiedCoord()) {
                    totalSAutoPref += scolFrame->GetPrefCoord();
                }
                float scolPct = scolFrame->GetPrefPercent();
                if (scolPct == 0.0f) {
                    totalSNonPctPref += scolFrame->GetPrefCoord();
                    ++nonPctCount;
                } else {
                    info.prefPercent -= scolPct;
                }
                info.minCoord -= scolFrame->GetMinCoord();
                info.prefCoord -= scolFrame->GetPrefCoord();
            }

            if (info.minCoord < 0)
                info.minCoord = 0;
            if (info.prefCoord < 0)
                info.prefCoord = 0;
            if (info.prefPercent < 0.0f)
                info.prefPercent = 0.0f;

            
            
            
            nscoord minWithinPref =
                PR_MIN(info.minCoord, totalSPref - totalSMin);
            NS_ASSERTION(minWithinPref >= 0, "neither value can be negative");
            nscoord minOutsidePref = info.minCoord - minWithinPref;

            
            
            const PRBool spanHasNonPctPref = totalSNonPctPref > 0;
            const PRBool spanHasPref = totalSPref > 0;
            const PRBool spanHasNonPct = nonPctCount > 0;

            
            
            
            for (scol = col, scol_end = col + colSpan;
                 scol < scol_end; ++scol) {
                nsTableColFrame *scolFrame = tableFrame->GetColFrame(scol);
                if (!scolFrame) {
                    NS_ERROR("column frames out of sync with cell map");
                    continue;
                }

                
                
                
                float allocatedPct = 0.0f;
                if (scolFrame->GetPrefPercent() == 0.0f &&
                    info.prefPercent != 0.0f) {
                    NS_ASSERTION((!spanHasNonPctPref ||
                                  totalSNonPctPref != 0) &&
                                 nonPctCount != 0,
                                 "should not be zero if we haven't allocated "
                                 "all pref percent");
                    if (spanHasNonPctPref) {
                        
                        
                        allocatedPct = info.prefPercent *
                                           (float(scolFrame->GetPrefCoord()) /
                                            float(totalSNonPctPref));
                    } else {
                        
                        allocatedPct = info.prefPercent / float(nonPctCount);
                    }
                    scolFrame->AddSpanPrefPercent(allocatedPct);
                }

                
                
                float minRatio = 0.0f;
                if (minWithinPref > 0) {
                    minRatio = float(scolFrame->GetPrefCoord() -
                                     scolFrame->GetMinCoord()) /
                               float(totalSPref - totalSMin);
                }

                
                
                float coordRatio; 
                if (spanHasPref) {
                    if (scolFrame->GetPrefCoord() == 0) {
                        
                        
                        coordRatio = 0.0f;
                    } else if (totalSAutoPref == 0) {
                        
                        coordRatio = float(scolFrame->GetPrefCoord()) /
                                     float(totalSPref);
                    } else if (!scolFrame->GetHasSpecifiedCoord()) {
                        
                        coordRatio = float(scolFrame->GetPrefCoord()) /
                                     float(totalSAutoPref);
                    } else {
                        
                        coordRatio = 0.0f;
                    }
                } else {
                    
                    coordRatio = 1.0f / float(scol_end - scol);
                }

                
                
                nscoord allocatedMinWithinPref =
                    NSToCoordRound(float(minWithinPref) * minRatio);
                nscoord allocatedMinOutsidePref =
                    NSToCoordRound(float(minOutsidePref) * coordRatio);
                nscoord allocatedPref =
                    NSToCoordRound(float(info.prefCoord) * coordRatio);
                nscoord spanMin = scolFrame->GetMinCoord() +
                        allocatedMinWithinPref + allocatedMinOutsidePref;
                nscoord spanPref = scolFrame->GetPrefCoord() + allocatedPref;
                scolFrame->AddSpanCoords(spanMin, spanPref,
                                         info.hasSpecifiedWidth);

                
                
                
                minWithinPref -= allocatedMinWithinPref;
                minOutsidePref -= allocatedMinOutsidePref;
                info.prefCoord -= allocatedPref;
                info.prefPercent -= allocatedPct;
                totalSPref -= scolFrame->GetPrefCoord();
                totalSMin -= scolFrame->GetMinCoord();
                if (!scolFrame->GetHasSpecifiedCoord()) {
                    totalSAutoPref -= scolFrame->GetPrefCoord();
                }                
                if (scolFrame->GetPrefPercent() == 0.0f) {
                    totalSNonPctPref -= scolFrame->GetPrefCoord();
                    --nonPctCount;
                }
            }

            
            
            NS_ASSERTION(totalSPref == 0 && totalSMin == 0 &&
                         totalSNonPctPref == 0 && nonPctCount == 0 &&
                         minOutsidePref == 0 && minWithinPref == 0 &&
                         info.prefCoord == 0 &&
                         (info.prefPercent == 0.0f || !spanHasNonPct),
                         "didn't subtract all that we added");
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
    printf("ComputeColumnIntrinsicWidths spanning\n");
    mTableFrame->Dump(PR_FALSE, PR_TRUE, PR_FALSE);
#endif
}

void
BasicTableLayoutStrategy::ComputeIntrinsicWidths(nsIRenderingContext* aRenderingContext)
{
    ComputeColumnIntrinsicWidths(aRenderingContext);

    nsTableCellMap *cellMap = mTableFrame->GetCellMap();
    nscoord min = 0, pref = 0, max_small_pct_pref = 0, nonpct_pref_total = 0;
    float pct_total = 0.0f; 
    PRInt32 colCount = cellMap->GetColCount();
    nscoord spacing = mTableFrame->GetCellSpacingX();
    nscoord add = spacing; 
                           

    for (PRInt32 col = 0; col < colCount; ++col) {
        nsTableColFrame *colFrame = mTableFrame->GetColFrame(col);
        if (!colFrame) {
            NS_ERROR("column frames out of sync with cell map");
            continue;
        }
        if (mTableFrame->GetNumCellsOriginatingInCol(col)) {
            add += spacing;
        }
        min += colFrame->GetMinCoord();
        pref += colFrame->GetPrefCoord();

        
        
        float p = colFrame->GetPrefPercent();
        if (p > 0.0f) {
            nscoord new_small_pct_expand =
                nscoord(float(colFrame->GetPrefCoord()) / p);
            if (new_small_pct_expand > max_small_pct_pref) {
                max_small_pct_pref = new_small_pct_expand;
            }
            pct_total += p;
        } else {
            nonpct_pref_total += colFrame->GetPrefCoord();
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
        nscoord large_pct_pref = nscoord(float(nonpct_pref_total) /
                                         (1.0f - pct_total));
        if (large_pct_pref > pref_pct_expand)
            pref_pct_expand = large_pct_pref;
    }

    
    if (colCount > 0) {
        min += add;
        pref += add;
        pref_pct_expand += add;
    }

    mMinWidth = min;
    mPrefWidth = pref;
    mPrefWidthPctExpand = pref_pct_expand;
}

 void
BasicTableLayoutStrategy::MarkIntrinsicWidthsDirty()
{
    mMinWidth = NS_INTRINSIC_WIDTH_UNKNOWN;
    mPrefWidth = NS_INTRINSIC_WIDTH_UNKNOWN;
    mPrefWidthPctExpand = NS_INTRINSIC_WIDTH_UNKNOWN;
    mLastCalcWidth = nscoord_MIN;
}

 void
BasicTableLayoutStrategy::ComputeColumnWidths(const nsHTMLReflowState& aReflowState)
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
        ComputeIntrinsicWidths(aReflowState.rendContext);

    nsTableCellMap *cellMap = mTableFrame->GetCellMap();
    PRInt32 colCount = cellMap->GetColCount();
    if (colCount <= 0)
        return; 

    nscoord spacing = mTableFrame->GetCellSpacingX();

    nscoord min = mMinWidth;

    
    nscoord subtract = spacing;
    for (PRInt32 col = 0; col < colCount; ++col) {
        if (mTableFrame->GetNumCellsOriginatingInCol(col)) {
            subtract += spacing;
        }
    }
    width -= subtract;
    min -= subtract;

    

    








































    
    

    nscoord guess_min = 0,
            guess_min_pct = 0,
            guess_min_spec = 0,
            guess_pref = 0,
            total_flex_pref = 0,
            total_fixed_pref = 0;
    float total_pct = 0.0f; 

    PRInt32 col;
    for (col = 0; col < colCount; ++col) {
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
            nscoord val = nscoord(float(width) * pct);
            if (val < min_width)
                val = min_width;
            guess_min_pct += val;
            guess_pref += val;
        } else {
            nscoord pref_width = colFrame->GetPrefCoord();
            guess_pref += pref_width;
            guess_min_pct += min_width;
            if (colFrame->GetHasSpecifiedCoord()) {
                
                
                guess_min_spec += pref_width - min_width;
                total_fixed_pref += pref_width;
            } else {
                total_flex_pref += pref_width;
            }
        }
    }
    guess_min_spec += guess_min_pct;

    
    enum Loop2Type {
        FLEX_PCT_SMALL, 
        FLEX_FIXED_SMALL, 
        FLEX_FLEX_SMALL, 
        FLEX_FLEX_LARGE, 
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
    if (width < guess_pref) {
        NS_ASSERTION(width >= guess_min, "bad width");
        if (width < guess_min_pct) {
            l2t = FLEX_PCT_SMALL;
            space = width - guess_min;
            basis.c = guess_min_pct - guess_min;
        } else if (width < guess_min_spec) {
            l2t = FLEX_FIXED_SMALL;
            space = width - guess_min_pct;
            basis.c = guess_min_spec - guess_min_pct;
        } else {
            l2t = FLEX_FLEX_SMALL;
            space = width - guess_min_spec;
            basis.c = guess_pref - guess_min_spec;
        }
    } else {
        space = width - guess_pref;
        if (total_flex_pref > 0) {
            l2t = FLEX_FLEX_LARGE;
            basis.c = total_flex_pref;
        } else if (total_fixed_pref > 0) {
            l2t = FLEX_FIXED_LARGE;
            basis.c = total_fixed_pref;
        } else if (total_pct > 0.0f) {
            l2t = FLEX_PCT_LARGE;
            basis.f = total_pct;
        } else {
            l2t = FLEX_ALL_LARGE;
            basis.c = colCount;
        }
    }

#ifdef DEBUG_dbaron_off
    printf("ComputeColumnWidths: %d columns in width %d,\n"
           "  guesses=[%d,%d,%d,%d], totals=[%d,%d,%f],\n"
           "  l2t=%d, space=%d, basis.c=%d\n",
           colCount, width,
           guess_min, guess_min_pct, guess_min_spec, guess_pref,
           total_flex_pref, total_fixed_pref, total_pct,
           l2t, space, basis.c);
#endif

    for (col = 0; col < colCount; ++col) {
        nsTableColFrame *colFrame = mTableFrame->GetColFrame(col);
        if (!colFrame) {
            NS_ERROR("column frames out of sync with cell map");
            continue;
        }
        nscoord col_width;

        float pct = colFrame->GetPrefPercent();
        if (pct != 0.0f) {
            col_width = nscoord(float(width) * pct);
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
                        nscoord(float(width) * pct) - col_width;
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
                    nscoord pref_minus_min = col_width - col_min;
                    col_width = col_width_before_adjust = col_min;
                    if (pref_minus_min != 0) {
                        float c = float(space) / float(basis.c);
                        basis.c -= pref_minus_min;
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
                        float c = float(space) / float(basis.c);
                        basis.c -= col_width;
                        col_width += NSToCoordRound(float(col_width) * c);
                    }
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

        space -= col_width - col_width_before_adjust;

        NS_ASSERTION(col_width >= colFrame->GetMinCoord(),
                     "assigned width smaller than min");

        nscoord old_final = colFrame->GetFinalWidth();
        colFrame->SetFinalWidth(col_width);

        if (old_final != col_width)
            mTableFrame->DidResizeColumns();
    }
    NS_ASSERTION(space == 0 &&
                 ((l2t == FLEX_PCT_LARGE)
                    ? (-0.001f < basis.f && basis.f < 0.001f)
                    : (basis.c == 0)),
                 "didn't subtract all that we added");
#ifdef DEBUG_TABLE_STRATEGY
    printf("ComputeColumnWidths final\n");
    mTableFrame->Dump(PR_FALSE, PR_TRUE, PR_FALSE);
#endif
}
