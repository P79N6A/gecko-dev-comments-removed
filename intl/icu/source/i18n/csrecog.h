






#ifndef __CSRECOG_H
#define __CSRECOG_H

#include "unicode/uobject.h"

#if !UCONFIG_NO_CONVERSION

#include "inputext.h"

U_NAMESPACE_BEGIN

class CharsetMatch;

class CharsetRecognizer : public UMemory
{
 public:
    







    virtual const char *getName() const = 0;
    
    



    virtual const char *getLanguage() const;
        
    






    virtual UBool match(InputText *textIn, CharsetMatch *results) const = 0;

    virtual ~CharsetRecognizer();
};

U_NAMESPACE_END

#endif
#endif 
