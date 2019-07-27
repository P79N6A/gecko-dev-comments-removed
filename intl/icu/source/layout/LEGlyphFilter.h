





#ifndef __LEGLYPHFILTER__H
#define __LEGLYPHFILTER__H

#include "LETypes.h"

U_NAMESPACE_BEGIN

#ifndef U_HIDE_INTERNAL_API






class LEGlyphFilter  {
public:
    



    virtual ~LEGlyphFilter();

    










    virtual le_bool accept(LEGlyphID glyph) const = 0;
};
#endif  

U_NAMESPACE_END
#endif
