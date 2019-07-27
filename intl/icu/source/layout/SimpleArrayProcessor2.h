





#ifndef __SIMPLEARRAYPROCESSOR2_H
#define __SIMPLEARRAYPROCESSOR2_H






#include "LETypes.h"
#include "MorphTables.h"
#include "SubtableProcessor2.h"
#include "NonContextualGlyphSubst.h"
#include "NonContextualGlyphSubstProc2.h"

U_NAMESPACE_BEGIN

class LEGlyphStorage;

class SimpleArrayProcessor2 : public NonContextualGlyphSubstitutionProcessor2
{
public:
    virtual void process(LEGlyphStorage &glyphStorage, LEErrorCode &success);

    SimpleArrayProcessor2(const LEReferenceTo<MorphSubtableHeader2> &morphSubtableHeader, LEErrorCode &success);

    virtual ~SimpleArrayProcessor2();

    




    virtual UClassID getDynamicClassID() const;

    




    static UClassID getStaticClassID();

private:
    SimpleArrayProcessor2();

protected:
    LEReferenceTo<SimpleArrayLookupTable> simpleArrayLookupTable;
    LEReferenceToArrayOf<LookupValue> valueArray;

};

U_NAMESPACE_END
#endif

