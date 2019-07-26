

















#ifndef __USPOOF_BUILDCONF_H__
#define __USPOOF_BUILDCONF_H__

#if !UCONFIG_NO_NORMALIZATION

#if !UCONFIG_NO_REGULAR_EXPRESSIONS 

#include "uspoof_impl.h"

U_NAMESPACE_BEGIN






struct SPUString : public UMemory {
    UnicodeString  *fStr;             
    int32_t         fStrTableIndex;   
                                      
                                      
    SPUString(UnicodeString *s);
    ~SPUString();
};










class SPUStringPool : public UMemory {
  public:
    SPUStringPool(UErrorCode &status);
    ~SPUStringPool();
    
    
    
    
    SPUString *addString(UnicodeString *src, UErrorCode &status);


    
    SPUString *getByIndex(int32_t i);

    
    void sort(UErrorCode &status);

    int32_t size();

  private:
    UVector     *fVec;    
    UHashtable  *fHash;   
};







class ConfusabledataBuilder : public UMemory {
  private:
    SpoofImpl  *fSpoofImpl;
    UChar      *fInput;
    UHashtable *fSLTable;
    UHashtable *fSATable; 
    UHashtable *fMLTable; 
    UHashtable *fMATable;
    UnicodeSet *fKeySet;     

    
    
    UVector            *fKeyVec;
    UVector            *fValueVec;
    UnicodeString      *fStringTable;
    UVector            *fStringLengthsTable;
    
    SPUStringPool      *stringPool;
    URegularExpression *fParseLine;
    URegularExpression *fParseHexNum;
    int32_t             fLineNum;

    ConfusabledataBuilder(SpoofImpl *spImpl, UErrorCode &status);
    ~ConfusabledataBuilder();
    void build(const char * confusables, int32_t confusablesLen, UErrorCode &status);

    
    
    
    void addKeyEntry(UChar32     keyChar,     
                     UHashtable *table,       
                     int32_t     tableFlag,   
                     UErrorCode &status);

    
    
    UnicodeString getMapping(int32_t index);

    
    void outputData(UErrorCode &status);

  public:
    static void buildConfusableData(SpoofImpl *spImpl, const char * confusables,
        int32_t confusablesLen, int32_t *errorType, UParseError *pe, UErrorCode &status);
};
U_NAMESPACE_END

#endif
#endif  
#endif  
