





#ifndef __SINGLETABLEPROCESSOR_H
#define __SINGLETABLEPROCESSOR_H






#include "LETypes.h"
#include "MorphTables.h"
#include "SubtableProcessor.h"
#include "NonContextualGlyphSubst.h"
#include "NonContextualGlyphSubstProc.h"

U_NAMESPACE_BEGIN

class LEGlyphStorage;

class SingleTableProcessor : public NonContextualGlyphSubstitutionProcessor
{
public:
    virtual void process(LEGlyphStorage &glyphStorage, LEErrorCode &success);

    SingleTableProcessor(const LEReferenceTo<MorphSubtableHeader> &morphSubtableHeader, LEErrorCode &success);

    virtual ~SingleTableProcessor();

    




    virtual UClassID getDynamicClassID() const;

    




    static UClassID getStaticClassID();

private:
    SingleTableProcessor();

protected:
    LEReferenceTo<SingleTableLookupTable> singleTableLookupTable;

};

U_NAMESPACE_END
#endif
