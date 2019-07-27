






#include "LETypes.h"
#include "OpenTypeUtilities.h"
#include "OpenTypeTables.h"
#include "ICUFeatures.h"
#include "LESwaps.h"

U_NAMESPACE_BEGIN

LEReferenceTo<FeatureTable> FeatureListTable::getFeatureTable(const LETableReference &base, le_uint16 featureIndex, LETag *featureTag, LEErrorCode &success) const
{
    LEReferenceToArrayOf<FeatureRecord>
        featureRecordArrayRef(base, success, featureRecordArray, featureIndex);

  if (featureIndex >= SWAPW(featureCount) || LE_FAILURE(success)) {
    return LEReferenceTo<FeatureTable>();
  }

    Offset featureTableOffset = featureRecordArray[featureIndex].featureTableOffset;

    *featureTag = SWAPT(featureRecordArray[featureIndex].featureTag);

    return LEReferenceTo<FeatureTable>(base, success, SWAPW(featureTableOffset));
}

#if 0








const FeatureTable *FeatureListTable::getFeatureTable(LETag featureTag) const
{
#if 0
    Offset featureTableOffset =
        OpenTypeUtilities::getTagOffset(featureTag, (TagAndOffsetRecord *) featureRecordArray, SWAPW(featureCount));

    if (featureTableOffset == 0) {
        return 0;
    }

    return (const FeatureTable *) ((char *) this + SWAPW(featureTableOffset));
#else
    int count = SWAPW(featureCount);
    
    for (int i = 0; i < count; i += 1) {
        if (SWAPT(featureRecordArray[i].featureTag) == featureTag) {
            return (const FeatureTable *) ((char *) this + SWAPW(featureRecordArray[i].featureTableOffset));
        }
    }

    return 0;
#endif
}
#endif

U_NAMESPACE_END
