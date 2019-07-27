





#include "LETypes.h"
#include "LayoutTables.h"
#include "LookupTables.h"
#include "LESwaps.h"

U_NAMESPACE_BEGIN













 
const LookupSegment *BinarySearchLookupTable::lookupSegment(const LETableReference &base, const LookupSegment *segments, LEGlyphID glyph, LEErrorCode &success) const
{
    
    le_int16  unity = SWAPW(unitSize);
    le_int16  probe = SWAPW(searchRange);
    le_int16  extra = SWAPW(rangeShift);
    TTGlyphID ttGlyph = (TTGlyphID) LE_GET_GLYPH(glyph);
    LEReferenceTo<LookupSegment> entry(base, success, segments);
    LEReferenceTo<LookupSegment> trial(entry, success, extra);

    if(LE_FAILURE(success)) return NULL;

    if (SWAPW(trial->lastGlyph) <= ttGlyph) {
        entry = trial;
    }

    while (probe > unity && LE_SUCCESS(success)) {
        probe >>= 1;
        trial = entry; 
        trial.addOffset(probe, success);

        if (SWAPW(trial->lastGlyph) <= ttGlyph) {
            entry = trial;
        }
    }

    if (SWAPW(entry->firstGlyph) <= ttGlyph) {
      return entry.getAlias();
    }

    return NULL;
}

const LookupSingle *BinarySearchLookupTable::lookupSingle(const LETableReference &base, const LookupSingle *entries, LEGlyphID glyph, LEErrorCode &success) const
{
    le_int16  unity = SWAPW(unitSize);
    le_int16  probe = SWAPW(searchRange);
    le_int16  extra = SWAPW(rangeShift);
    TTGlyphID ttGlyph = (TTGlyphID) LE_GET_GLYPH(glyph);
    LEReferenceTo<LookupSingle> entry(base, success, entries);
    LEReferenceTo<LookupSingle> trial(entry, success, extra);

    if (SWAPW(trial->glyph) <= ttGlyph) {
        entry = trial;
    }

    while (probe > unity && LE_SUCCESS(success)) {
        probe >>= 1;
        trial = entry;
        trial.addOffset(probe, success);

        if (SWAPW(trial->glyph) <= ttGlyph) {
            entry = trial;
        }
    }

    if (SWAPW(entry->glyph) == ttGlyph) {
      return entry.getAlias();
    }

    return NULL;
}

U_NAMESPACE_END
