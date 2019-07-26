
















#ifndef REGEXST_H
#define REGEXST_H

#include "unicode/utypes.h"
#include "unicode/utext.h"
#if !UCONFIG_NO_REGULAR_EXPRESSIONS

#include "regeximp.h"

U_NAMESPACE_BEGIN

class  UnicodeSet;


class RegexStaticSets : public UMemory {
public:
    static RegexStaticSets *gStaticSets;  
                                          

    RegexStaticSets(UErrorCode *status);         
    ~RegexStaticSets();
    static void    initGlobals(UErrorCode *status);
    static UBool   cleanup();

    UnicodeSet    *fPropSets[URX_LAST_SET];     
    Regex8BitSet   fPropSets8[URX_LAST_SET];    

    UnicodeSet    fRuleSets[10];               
    UnicodeSet    fUnescapeCharSet;            
                                               
    UnicodeSet    *fRuleDigitsAlias;
    UText         *fEmptyText;                 
                                               

};


U_NAMESPACE_END
#endif   
#endif   

