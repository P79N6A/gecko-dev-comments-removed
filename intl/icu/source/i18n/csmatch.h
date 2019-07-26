






#ifndef __CSMATCH_H
#define __CSMATCH_H

#include "unicode/uobject.h"

#if !UCONFIG_NO_CONVERSION

U_NAMESPACE_BEGIN

class InputText;
class CharsetRecognizer;












class CharsetMatch : public UMemory
{
 private:
    InputText               *textIn;
    int32_t                  confidence;
    const char              *fCharsetName;
    const char              *fLang;

 public:
    CharsetMatch();

    





    void set(InputText               *input, 
             const CharsetRecognizer *cr, 
             int32_t                  conf, 
             const char              *csName=NULL, 
             const char              *lang=NULL);

    


    const char *getName() const;

    const char *getLanguage()const;

    int32_t getConfidence()const;

    int32_t getUChars(UChar *buf, int32_t cap, UErrorCode *status) const;
};

U_NAMESPACE_END

#endif
#endif 
