


















#ifndef __USPOOF_BUILDWSCONF_H__
#define __USPOOF_BUILDWSCONF_H__

#include "unicode/utypes.h"

#if !UCONFIG_NO_NORMALIZATION

#if !UCONFIG_NO_REGULAR_EXPRESSIONS 

#include "uspoof_impl.h"
#include "utrie2.h"


U_NAMESPACE_BEGIN







class BuilderScriptSet: public UMemory {
  public:
    UChar32      codePoint;       
    UTrie2      *trie;            
                                  
                                  
                                  
    ScriptSet   *sset;            

                                  
    uint32_t     index;           
                                  
    uint32_t     rindex;          
                                  
    UBool        scriptSetOwned;  
                                  

    BuilderScriptSet();
    ~BuilderScriptSet();
};


void buildWSConfusableData(SpoofImpl *spImpl, const char * confusablesWS,
          int32_t confusablesWSLen, UParseError *pe, UErrorCode &status); 

U_NAMESPACE_END

#endif 
#endif 
#endif
