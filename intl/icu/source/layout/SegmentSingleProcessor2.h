





#ifndef __SEGMENTSINGLEPROCESSOR_H
#define __SEGMENTSINGLEPROCESSOR_H






#include "LETypes.h"
#include "MorphTables.h"
#include "SubtableProcessor2.h"
#include "NonContextualGlyphSubst.h"
#include "NonContextualGlyphSubstProc2.h"

U_NAMESPACE_BEGIN

class LEGlyphStorage;

class SegmentSingleProcessor2 : public NonContextualGlyphSubstitutionProcessor2
{
public:
    virtual void process(LEGlyphStorage &glyphStorage, LEErrorCode &success);

    SegmentSingleProcessor2(const LEReferenceTo<MorphSubtableHeader2> &morphSubtableHeader, LEErrorCode &success);

    virtual ~SegmentSingleProcessor2();

    




    virtual UClassID getDynamicClassID() const;

    




    static UClassID getStaticClassID();

private:
    SegmentSingleProcessor2();

protected:
    LEReferenceTo<SegmentSingleLookupTable> segmentSingleLookupTable;

};

U_NAMESPACE_END
#endif

