





#ifndef __NONCONTEXTUALGLYPHSUBSTITUTIONPROCESSOR2_H
#define __NONCONTEXTUALGLYPHSUBSTITUTIONPROCESSOR2_H






#include "LETypes.h"
#include "MorphTables.h"
#include "SubtableProcessor2.h"
#include "NonContextualGlyphSubst.h"

U_NAMESPACE_BEGIN

class LEGlyphStorage;

class NonContextualGlyphSubstitutionProcessor2 : public SubtableProcessor2
{
public:
    virtual void process(LEGlyphStorage &glyphStorage, LEErrorCode &success) = 0;

    static SubtableProcessor2 *createInstance(const LEReferenceTo<MorphSubtableHeader2> &morphSubtableHeader, LEErrorCode &success);

protected:
    NonContextualGlyphSubstitutionProcessor2();
    NonContextualGlyphSubstitutionProcessor2(const LEReferenceTo<MorphSubtableHeader2> &morphSubtableHeader, LEErrorCode &success);

    virtual ~NonContextualGlyphSubstitutionProcessor2();

private:
    NonContextualGlyphSubstitutionProcessor2(const NonContextualGlyphSubstitutionProcessor2 &other); 
    NonContextualGlyphSubstitutionProcessor2 &operator=(const NonContextualGlyphSubstitutionProcessor2 &other); 
};

U_NAMESPACE_END
#endif
