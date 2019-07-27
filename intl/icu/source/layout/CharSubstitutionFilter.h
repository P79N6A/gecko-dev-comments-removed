





#ifndef __CHARSUBSTITUTIONFILTER_H
#define __CHARSUBSTITUTIONFILTER_H

#include "LETypes.h"
#include "LEGlyphFilter.h"

U_NAMESPACE_BEGIN

class LEFontInstance;







class CharSubstitutionFilter : public UMemory, public LEGlyphFilter
{
private:
    




    const LEFontInstance *fFontInstance;

    




    CharSubstitutionFilter(const CharSubstitutionFilter &other); 

    




    CharSubstitutionFilter &operator=(const CharSubstitutionFilter &other); 

public:
    






    CharSubstitutionFilter(const LEFontInstance *fontInstance);

    




    ~CharSubstitutionFilter();

    










    le_bool accept(LEGlyphID glyph) const;
};

U_NAMESPACE_END
#endif


