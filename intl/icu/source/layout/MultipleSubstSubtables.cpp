





#include "LETypes.h"
#include "LEGlyphFilter.h"
#include "OpenTypeTables.h"
#include "GlyphSubstitutionTables.h"
#include "MultipleSubstSubtables.h"
#include "GlyphIterator.h"
#include "LESwaps.h"

U_NAMESPACE_BEGIN

le_uint32 MultipleSubstitutionSubtable::process(const LETableReference &base, GlyphIterator *glyphIterator, LEErrorCode& success, const LEGlyphFilter *filter) const
{
    if (LE_FAILURE(success)) {
        return 0;
    }

    LEGlyphID glyph = glyphIterator->getCurrGlyphID();

    
    
    
    
    
    
    
    if (filter != NULL && filter->accept(glyph)) {
        return 0;
    }

    le_int32 coverageIndex = getGlyphCoverage(base, glyph, success);
    le_uint16 seqCount = SWAPW(sequenceCount);
    LEReferenceToArrayOf<Offset>
        sequenceTableOffsetArrayRef(base, success, sequenceTableOffsetArray, seqCount);

    if (LE_FAILURE(success)) {
        return 0;
    }
    if (coverageIndex >= 0 && coverageIndex < seqCount) {
        Offset sequenceTableOffset = SWAPW(sequenceTableOffsetArray[coverageIndex]);
        const SequenceTable *sequenceTable = (const SequenceTable *) ((char *) this + sequenceTableOffset);
        le_uint16 glyphCount = SWAPW(sequenceTable->glyphCount);

        if (glyphCount == 0) {
            glyphIterator->setCurrGlyphID(0xFFFF);
            return 1;
        } else if (glyphCount == 1) {
            TTGlyphID substitute = SWAPW(sequenceTable->substituteArray[0]);

            if (filter != NULL && ! filter->accept(LE_SET_GLYPH(glyph, substitute))) {
                return 0;
            }

            glyphIterator->setCurrGlyphID(substitute);
            return 1;
        } else {
            
            
            if (filter != NULL) {
                for (le_int32 i = 0; i < glyphCount; i += 1) {
                    TTGlyphID substitute = SWAPW(sequenceTable->substituteArray[i]);

                    if (! filter->accept(substitute)) {
                        return 0;
                    }
                }
            }

            LEGlyphID *newGlyphs = glyphIterator->insertGlyphs(glyphCount, success);
            if (LE_FAILURE(success)) { 
                return 0;
            }

            le_int32 insert = 0, direction = 1;

            if (glyphIterator->isRightToLeft()) {
                insert = glyphCount - 1;
                direction = -1;
            }

            for (le_int32 i = 0; i < glyphCount; i += 1) {
                TTGlyphID substitute = SWAPW(sequenceTable->substituteArray[i]);

                newGlyphs[insert] = LE_SET_GLYPH(glyph, substitute);
                insert += direction;
            }

            return 1;
        }
    }

    return 0;
}

U_NAMESPACE_END
