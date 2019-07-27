





#ifndef __SEGMENTSINGLEPROCESSOR_H
#define __SEGMENTSINGLEPROCESSOR_H






#include "LETypes.h"
#include "MorphTables.h"
#include "SubtableProcessor.h"
#include "NonContextualGlyphSubst.h"
#include "NonContextualGlyphSubstProc.h"

U_NAMESPACE_BEGIN

class LEGlyphStorage;

class SegmentSingleProcessor : public NonContextualGlyphSubstitutionProcessor
{
public:
    virtual void process(LEGlyphStorage &glyphStorage, LEErrorCode &success);

    SegmentSingleProcessor(const LEReferenceTo<MorphSubtableHeader> &morphSubtableHeader, LEErrorCode &success);

    virtual ~SegmentSingleProcessor();

    




    virtual UClassID getDynamicClassID() const;

    




    static UClassID getStaticClassID();

private:
    SegmentSingleProcessor();

protected:
    LEReferenceTo<SegmentSingleLookupTable> segmentSingleLookupTable;

};

U_NAMESPACE_END
#endif

