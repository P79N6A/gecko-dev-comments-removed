












#ifndef SMPDTFST_H
#define SMPDTFST_H

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/udat.h"

U_NAMESPACE_BEGIN

class  UnicodeSet;


class SimpleDateFormatStaticSets : public UMemory
{
public:
    static SimpleDateFormatStaticSets *gStaticSets;  
    
    
    SimpleDateFormatStaticSets(UErrorCode *status);
    ~SimpleDateFormatStaticSets();
    
    static void    initSets(UErrorCode *status);
    static UBool   cleanup();
    
    static UnicodeSet *getIgnorables(UDateFormatField fieldIndex);
    
private:
    UnicodeSet *fDateIgnorables;
    UnicodeSet *fTimeIgnorables;
    UnicodeSet *fOtherIgnorables;
};


U_NAMESPACE_END

#endif   
#endif   
