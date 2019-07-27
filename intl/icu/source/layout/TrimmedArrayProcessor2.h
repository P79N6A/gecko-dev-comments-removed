





#ifndef __TRIMMEDARRAYPROCESSOR2_H
#define __TRIMMEDARRAYPROCESSOR2_H






#include "LETypes.h"
#include "MorphTables.h"
#include "SubtableProcessor2.h"
#include "NonContextualGlyphSubst.h"
#include "NonContextualGlyphSubstProc2.h"

U_NAMESPACE_BEGIN

class LEGlyphStorage;

class TrimmedArrayProcessor2 : public NonContextualGlyphSubstitutionProcessor2
{
public:
    virtual void process(LEGlyphStorage &glyphStorage, LEErrorCode &success);

    TrimmedArrayProcessor2(const LEReferenceTo<MorphSubtableHeader2> &morphSubtableHeader, LEErrorCode &success);

    virtual ~TrimmedArrayProcessor2();

    




    virtual UClassID getDynamicClassID() const;

    




    static UClassID getStaticClassID();

private:
    TrimmedArrayProcessor2();

protected:
    TTGlyphID firstGlyph;
    TTGlyphID lastGlyph;
    LEReferenceTo<TrimmedArrayLookupTable> trimmedArrayLookupTable;
    LEReferenceToArrayOf<LookupValue> valueArray;
};

U_NAMESPACE_END
#endif

