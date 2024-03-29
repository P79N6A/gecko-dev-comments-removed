






#ifndef UDISPLAYCONTEXT_H
#define UDISPLAYCONTEXT_H

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING











enum UDisplayContextType {
    




    UDISPCTX_TYPE_DIALECT_HANDLING = 0,
    





    UDISPCTX_TYPE_CAPITALIZATION = 1
#ifndef U_HIDE_DRAFT_API
    ,
    




    UDISPCTX_TYPE_DISPLAY_LENGTH = 2
#endif  
};



typedef enum UDisplayContextType UDisplayContextType;






enum UDisplayContext {
    





    





    UDISPCTX_STANDARD_NAMES = (UDISPCTX_TYPE_DIALECT_HANDLING<<8) + 0,
    





    UDISPCTX_DIALECT_NAMES = (UDISPCTX_TYPE_DIALECT_HANDLING<<8) + 1,
    








    



    UDISPCTX_CAPITALIZATION_NONE = (UDISPCTX_TYPE_CAPITALIZATION<<8) + 0,
    




    UDISPCTX_CAPITALIZATION_FOR_MIDDLE_OF_SENTENCE = (UDISPCTX_TYPE_CAPITALIZATION<<8) + 1,
    




    UDISPCTX_CAPITALIZATION_FOR_BEGINNING_OF_SENTENCE = (UDISPCTX_TYPE_CAPITALIZATION<<8) + 2,
    




    UDISPCTX_CAPITALIZATION_FOR_UI_LIST_OR_MENU = (UDISPCTX_TYPE_CAPITALIZATION<<8) + 3,
    





    UDISPCTX_CAPITALIZATION_FOR_STANDALONE = (UDISPCTX_TYPE_CAPITALIZATION<<8) + 4
#ifndef U_HIDE_DRAFT_API
    ,
    





    





    UDISPCTX_LENGTH_FULL = (UDISPCTX_TYPE_DISPLAY_LENGTH<<8) + 0,
    





    UDISPCTX_LENGTH_SHORT = (UDISPCTX_TYPE_DISPLAY_LENGTH<<8) + 1
#endif  
};



typedef enum UDisplayContext UDisplayContext;

#endif 

#endif
