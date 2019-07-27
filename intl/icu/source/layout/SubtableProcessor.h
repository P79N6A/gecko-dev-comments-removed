





#ifndef __SUBTABLEPROCESSOR_H
#define __SUBTABLEPROCESSOR_H






#include "LETypes.h"
#include "MorphTables.h"

U_NAMESPACE_BEGIN

class LEGlyphStorage;

class SubtableProcessor : public UMemory {
public:
    virtual void process(LEGlyphStorage &glyphStorage, LEErrorCode &success) = 0;
    virtual ~SubtableProcessor();

protected:
    SubtableProcessor(const LEReferenceTo<MorphSubtableHeader> &morphSubtableHeader, LEErrorCode &success);

    SubtableProcessor();

    le_int16 length;
    SubtableCoverage coverage;
    FeatureFlags subtableFeatures;

    const LEReferenceTo<MorphSubtableHeader> subtableHeader;

private:

    SubtableProcessor(const SubtableProcessor &other); 
    SubtableProcessor &operator=(const SubtableProcessor &other); 
};

U_NAMESPACE_END
#endif

