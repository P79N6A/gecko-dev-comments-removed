





#include "LETypes.h"
#include "LEFontInstance.h"
#include "OpenTypeTables.h"
#include "AnchorTables.h"
#include "MarkArrays.h"
#include "GlyphPositioningTables.h"
#include "AttachmentPosnSubtables.h"
#include "MarkToBasePosnSubtables.h"
#include "GlyphIterator.h"
#include "LESwaps.h"

U_NAMESPACE_BEGIN

LEGlyphID MarkToBasePositioningSubtable::findBaseGlyph(GlyphIterator *glyphIterator) const
{
    if (glyphIterator->prev()) {
        return glyphIterator->getCurrGlyphID();
    }

    return 0xFFFF;
}

le_int32 MarkToBasePositioningSubtable::process(const LETableReference &base, GlyphIterator *glyphIterator, const LEFontInstance *fontInstance, LEErrorCode &success) const
{
    LEGlyphID markGlyph = glyphIterator->getCurrGlyphID();
    le_int32 markCoverage = getGlyphCoverage(base, (LEGlyphID) markGlyph, success);

    if (markCoverage < 0) {
        
        return 0;
    }

    LEPoint markAnchor;
    const MarkArray *markArray = (const MarkArray *) ((char *) this + SWAPW(markArrayOffset));
    le_int32 markClass = markArray->getMarkClass(markGlyph, markCoverage, fontInstance, markAnchor);
    le_uint16 mcCount = SWAPW(classCount);

    if (markClass < 0 || markClass >= mcCount) {
        
        
        return 0;
    }

    
    GlyphIterator baseIterator(*glyphIterator, (le_uint16) (lfIgnoreMarks ));
    LEGlyphID baseGlyph = findBaseGlyph(&baseIterator);
    le_int32 baseCoverage = getBaseCoverage(base, (LEGlyphID) baseGlyph, success);
    const BaseArray *baseArray = (const BaseArray *) ((char *) this + SWAPW(baseArrayOffset));
    le_uint16 baseCount = SWAPW(baseArray->baseRecordCount);

    if (baseCoverage < 0 || baseCoverage >= baseCount) {
        
        
        
        return 0;
    }

    const BaseRecord *baseRecord = &baseArray->baseRecordArray[baseCoverage * mcCount];
    Offset anchorTableOffset = SWAPW(baseRecord->baseAnchorTableOffsetArray[markClass]);
    const AnchorTable *anchorTable = (const AnchorTable *) ((char *) baseArray + anchorTableOffset);
    LEPoint baseAnchor, markAdvance, pixels;

    if (anchorTableOffset == 0) {
        
        glyphIterator->setCurrGlyphBaseOffset(baseIterator.getCurrStreamPosition());
        return 0;
    }

    anchorTable->getAnchor(baseGlyph, fontInstance, baseAnchor);

    fontInstance->getGlyphAdvance(markGlyph, pixels);
    fontInstance->pixelsToUnits(pixels, markAdvance);

    float anchorDiffX = baseAnchor.fX - markAnchor.fX;
    float anchorDiffY = baseAnchor.fY - markAnchor.fY;

    glyphIterator->setCurrGlyphBaseOffset(baseIterator.getCurrStreamPosition());

    if (glyphIterator->isRightToLeft()) {
        
        
        glyphIterator->setCurrGlyphPositionAdjustment(anchorDiffX, anchorDiffY, -markAdvance.fX, -markAdvance.fY);
    } else {
        LEPoint baseAdvance;

        fontInstance->getGlyphAdvance(baseGlyph, pixels);
        
        
        GlyphIterator gi(baseIterator, (le_uint16)0); 
        gi.next(); 
        while (gi.getCurrStreamPosition() < glyphIterator->getCurrStreamPosition()) { 
            LEGlyphID otherMark = gi.getCurrGlyphID();
            LEPoint px;
            fontInstance->getGlyphAdvance(otherMark, px); 
            pixels.fX += px.fX; 
            pixels.fY += px.fY;
            gi.next();
        }
        
   
        fontInstance->pixelsToUnits(pixels, baseAdvance);

        glyphIterator->setCurrGlyphPositionAdjustment(anchorDiffX - baseAdvance.fX, anchorDiffY - baseAdvance.fY, -markAdvance.fX, -markAdvance.fY);
    }

    return 1;
}

U_NAMESPACE_END
