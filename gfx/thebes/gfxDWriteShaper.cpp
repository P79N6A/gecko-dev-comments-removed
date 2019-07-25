




































#include "gfxDWriteShaper.h"
#include "gfxWindowsPlatform.h"

#include <dwrite.h>

#include "gfxDWriteTextAnalysis.h"

#include "nsCRT.h"

#define MAX_RANGE_LENGTH 25000

PRBool
gfxDWriteShaper::InitTextRun(gfxContext *aContext,
                             gfxTextRun *aTextRun,
                             const PRUnichar *aString,
                             PRUint32 aRunStart,
                             PRUint32 aRunLength,
                             PRInt32 aRunScript)
{
    HRESULT hr;
    

    DWRITE_READING_DIRECTION readingDirection = 
        aTextRun->IsRightToLeft()
            ? DWRITE_READING_DIRECTION_RIGHT_TO_LEFT
            : DWRITE_READING_DIRECTION_LEFT_TO_RIGHT;

    gfxDWriteFont *font = static_cast<gfxDWriteFont*>(mFont);

    gfxTextRun::CompressedGlyph g;

    nsRefPtr<IDWriteTextAnalyzer> analyzer;

    hr = gfxWindowsPlatform::GetPlatform()->GetDWriteFactory()->
        CreateTextAnalyzer(getter_AddRefs(analyzer));
    if (FAILED(hr)) {
        return PR_FALSE;
    }

    






    PRBool result = PR_TRUE;
    UINT32 rangeOffset = 0;
    while (rangeOffset < aRunLength) {
        PRUint32 rangeLen = NS_MIN<PRUint32>(aRunLength - rangeOffset,
                                             MAX_RANGE_LENGTH);
        if (rangeOffset + rangeLen < aRunLength) {
            
            
            
            
            
            
            PRUint32 adjRangeLen = 0;
            const PRUnichar *rangeStr = aString + aRunStart + rangeOffset;
            for (PRUint32 i = rangeLen; i > MAX_RANGE_LENGTH / 2; i--) {
                if (nsCRT::IsAsciiSpace(rangeStr[i])) {
                    adjRangeLen = i;
                    break;
                }
                if (adjRangeLen == 0 &&
                    aTextRun->IsClusterStart(aRunStart + rangeOffset + i)) {
                    adjRangeLen = i;
                }
            }
            if (adjRangeLen != 0) {
                rangeLen = adjRangeLen;
            }
        }

        PRUint32 rangeStart = aRunStart + rangeOffset;
        rangeOffset += rangeLen;
        TextAnalysis analysis(aString + rangeStart, rangeLen,
            NULL, 
            readingDirection);
        TextAnalysis::Run *runHead;
        DWRITE_LINE_BREAKPOINT *linebreaks;
        hr = analysis.GenerateResults(analyzer, &runHead, &linebreaks);

        if (FAILED(hr)) {
            NS_WARNING("Analyzer failed to generate results.");
            result = PR_FALSE;
            break;
        }

        PRUint32 appUnitsPerDevPixel = aTextRun->GetAppUnitsPerDevUnit();

        UINT32 maxGlyphs = 0;
trymoreglyphs:
        if ((PR_UINT32_MAX - 3 * rangeLen / 2 + 16) < maxGlyphs) {
            
            
            continue;
        }
        maxGlyphs += 3 * rangeLen / 2 + 16;

        nsAutoTArray<UINT16, 400> clusters;
        nsAutoTArray<UINT16, 400> indices;
        nsAutoTArray<DWRITE_SHAPING_TEXT_PROPERTIES, 400> textProperties;
        nsAutoTArray<DWRITE_SHAPING_GLYPH_PROPERTIES, 400> glyphProperties;
        if (!clusters.SetLength(rangeLen) ||
            !indices.SetLength(maxGlyphs) || 
            !textProperties.SetLength(maxGlyphs) ||
            !glyphProperties.SetLength(maxGlyphs)) {
                continue;
        }

        UINT32 actualGlyphs;

        hr = analyzer->GetGlyphs(aString + rangeStart, rangeLen,
            font->GetFontFace(), FALSE, 
            readingDirection == DWRITE_READING_DIRECTION_RIGHT_TO_LEFT,
            &runHead->mScript, NULL, NULL, NULL, NULL, 0,
            maxGlyphs, clusters.Elements(), textProperties.Elements(),
            indices.Elements(), glyphProperties.Elements(), &actualGlyphs);

        if (hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER)) {
            
            goto trymoreglyphs;
        }
        if (FAILED(hr)) {
            NS_WARNING("Analyzer failed to get glyphs.");
            result = PR_FALSE;
            break;
        }

        WORD gID = indices[0];
        nsAutoTArray<FLOAT, 400> advances;
        nsAutoTArray<DWRITE_GLYPH_OFFSET, 400> glyphOffsets;
        if (!advances.SetLength(actualGlyphs) || 
            !glyphOffsets.SetLength(actualGlyphs)) {
            continue;
        }

        if (!static_cast<gfxDWriteFont*>(mFont)->mUseSubpixelPositions) {
            hr = analyzer->GetGdiCompatibleGlyphPlacements(
                                              aString + rangeStart,
                                              clusters.Elements(),
                                              textProperties.Elements(),
                                              rangeLen,
                                              indices.Elements(),
                                              glyphProperties.Elements(),
                                              actualGlyphs,
                                              font->GetFontFace(),
                                              font->GetAdjustedSize(),
                                              1.0,
                                              nsnull,
                                              FALSE,
                                              FALSE,
                                              FALSE,
                                              &runHead->mScript,
                                              NULL,
                                              NULL,
                                              NULL,
                                              0,
                                              advances.Elements(),
                                              glyphOffsets.Elements());
        } else {
            hr = analyzer->GetGlyphPlacements(aString + rangeStart,
                                              clusters.Elements(),
                                              textProperties.Elements(),
                                              rangeLen,
                                              indices.Elements(),
                                              glyphProperties.Elements(),
                                              actualGlyphs,
                                              font->GetFontFace(),
                                              font->GetAdjustedSize(),
                                              FALSE,
                                              FALSE,
                                              &runHead->mScript,
                                              NULL,
                                              NULL,
                                              NULL,
                                              0,
                                              advances.Elements(),
                                              glyphOffsets.Elements());
        }
        if (FAILED(hr)) {
            NS_WARNING("Analyzer failed to get glyph placements.");
            result = PR_FALSE;
            break;
        }

        nsAutoTArray<gfxTextRun::DetailedGlyph,1> detailedGlyphs;

        for (unsigned int c = 0; c < rangeLen; c++) {
            PRUint32 k = clusters[c];
            PRUint32 absC = rangeStart + c;

            if (c > 0 && k == clusters[c - 1]) {
                g.SetComplex(aTextRun->IsClusterStart(absC), PR_FALSE, 0);
                aTextRun->SetGlyphs(absC, g, nsnull);
                
                continue;
            }

            
            PRUint32 glyphCount = actualGlyphs - k;
            PRUint32 nextClusterOffset;
            for (nextClusterOffset = c + 1; 
                nextClusterOffset < rangeLen; ++nextClusterOffset) {
                if (clusters[nextClusterOffset] > k) {
                    glyphCount = clusters[nextClusterOffset] - k;
                    break;
                }
            }
            PRInt32 advance = (PRInt32)(advances[k] * appUnitsPerDevPixel);
            if (glyphCount == 1 && advance >= 0 &&
                glyphOffsets[k].advanceOffset == 0 &&
                glyphOffsets[k].ascenderOffset == 0 &&
                aTextRun->IsClusterStart(absC) &&
                gfxTextRun::CompressedGlyph::IsSimpleAdvance(advance) &&
                gfxTextRun::CompressedGlyph::IsSimpleGlyphID(indices[k])) {
                  aTextRun->SetSimpleGlyph(absC, 
                                          g.SetSimpleGlyph(advance, 
                                                           indices[k]));
            } else {
                if (detailedGlyphs.Length() < glyphCount) {
                    if (!detailedGlyphs.AppendElements(
                        glyphCount - detailedGlyphs.Length())) {
                        continue;
                    }
                }
                float totalAdvance = 0;
                for (unsigned int z = 0; z < glyphCount; z++) {
                    detailedGlyphs[z].mGlyphID = indices[k + z];
                    detailedGlyphs[z].mAdvance = 
                        (PRInt32)(advances[k + z]
                           * appUnitsPerDevPixel);
                    if (readingDirection == 
                        DWRITE_READING_DIRECTION_RIGHT_TO_LEFT) {
                        detailedGlyphs[z].mXOffset = 
                            (totalAdvance + 
                              glyphOffsets[k + z].advanceOffset)
                            * appUnitsPerDevPixel;
                    } else {
                        detailedGlyphs[z].mXOffset = 
                            glyphOffsets[k + z].advanceOffset *
                            appUnitsPerDevPixel;
                    }
                    detailedGlyphs[z].mYOffset = 
                        -glyphOffsets[k + z].ascenderOffset *
                        appUnitsPerDevPixel;
                    totalAdvance += advances[k + z];
                }
                aTextRun->SetGlyphs(
                    absC,
                    g.SetComplex(aTextRun->IsClusterStart(absC),
                                 PR_TRUE,
                                 glyphCount),
                    detailedGlyphs.Elements());
            }
        }
    }

    return result;
}
