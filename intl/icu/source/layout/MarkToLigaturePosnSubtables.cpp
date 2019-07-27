




#include "LETypes.h"
#include "LEFontInstance.h"
#include "OpenTypeTables.h"
#include "AnchorTables.h"
#include "MarkArrays.h"
#include "GlyphPositioningTables.h"
#include "AttachmentPosnSubtables.h"
#include "MarkToLigaturePosnSubtables.h"
#include "GlyphIterator.h"
#include "LESwaps.h"

U_NAMESPACE_BEGIN

LEGlyphID MarkToLigaturePositioningSubtable::findLigatureGlyph(GlyphIterator *glyphIterator) const
{
    if (glyphIterator->prev()) {
        return glyphIterator->getCurrGlyphID();
    }

    return 0xFFFF;
}

le_int32 MarkToLigaturePositioningSubtable::process(const LETableReference &base, GlyphIterator *glyphIterator, const LEFontInstance *fontInstance, LEErrorCode &success) const
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

    
    GlyphIterator ligatureIterator(*glyphIterator, (le_uint16) (lfIgnoreMarks ));
    LEGlyphID ligatureGlyph = findLigatureGlyph(&ligatureIterator);
    le_int32 ligatureCoverage = getBaseCoverage(base, (LEGlyphID) ligatureGlyph, success);
    const LigatureArray *ligatureArray = (const LigatureArray *) ((char *) this + SWAPW(baseArrayOffset));
    le_uint16 ligatureCount = SWAPW(ligatureArray->ligatureCount);

    if (ligatureCoverage < 0 || ligatureCoverage >= ligatureCount) {
        
        
        
        return 0;
    }

    le_int32 markPosition = glyphIterator->getCurrStreamPosition();
    Offset ligatureAttachOffset = SWAPW(ligatureArray->ligatureAttachTableOffsetArray[ligatureCoverage]);
    const LigatureAttachTable *ligatureAttachTable = (const LigatureAttachTable *) ((char *) ligatureArray + ligatureAttachOffset);
    le_int32 componentCount = SWAPW(ligatureAttachTable->componentCount);
    le_int32 component = ligatureIterator.getMarkComponent(markPosition);

    if (component >= componentCount) {
        
        component = componentCount - 1;
    }

    const ComponentRecord *componentRecord = &ligatureAttachTable->componentRecordArray[component * mcCount];
    Offset anchorTableOffset = SWAPW(componentRecord->ligatureAnchorTableOffsetArray[markClass]);
    const AnchorTable *anchorTable = (const AnchorTable *) ((char *) ligatureAttachTable + anchorTableOffset);
    LEPoint ligatureAnchor, markAdvance, pixels;

    anchorTable->getAnchor(ligatureGlyph, fontInstance, ligatureAnchor);

    fontInstance->getGlyphAdvance(markGlyph, pixels);
    fontInstance->pixelsToUnits(pixels, markAdvance);

    float anchorDiffX = ligatureAnchor.fX - markAnchor.fX;
    float anchorDiffY = ligatureAnchor.fY - markAnchor.fY;

    glyphIterator->setCurrGlyphBaseOffset(ligatureIterator.getCurrStreamPosition());

    if (glyphIterator->isRightToLeft()) {
        glyphIterator->setCurrGlyphPositionAdjustment(anchorDiffX, anchorDiffY, -markAdvance.fX, -markAdvance.fY);
    } else {
        LEPoint ligatureAdvance;

        fontInstance->getGlyphAdvance(ligatureGlyph, pixels);
        fontInstance->pixelsToUnits(pixels, ligatureAdvance);

        glyphIterator->setCurrGlyphPositionAdjustment(anchorDiffX - ligatureAdvance.fX, anchorDiffY - ligatureAdvance.fY, -markAdvance.fX, -markAdvance.fY);
    }

    return 1;
}

U_NAMESPACE_END
