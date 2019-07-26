











#ifndef USPOOFIM_H
#define USPOOFIM_H

#include "unicode/utypes.h"
#include "unicode/uspoof.h"
#include "unicode/uscript.h"
#include "unicode/udata.h"

#include "utrie2.h"

#if !UCONFIG_NO_NORMALIZATION

#ifdef __cplusplus

U_NAMESPACE_BEGIN



#define USPOOF_MAX_SKELETON_EXPANSION 20



#define USPOOF_STACK_BUFFER_SIZE 100


#define USPOOF_MAGIC 0x3845fdef

class IdentifierInfo;
class ScriptSet;
class SpoofData;
struct SpoofDataHeader;
struct SpoofStringLengthsElement;





class SpoofImpl : public UObject  {
public:
	SpoofImpl(SpoofData *data, UErrorCode &status);
	SpoofImpl();
	virtual ~SpoofImpl();

    

    SpoofImpl(const SpoofImpl &src, UErrorCode &status);
    
    static SpoofImpl *validateThis(USpoofChecker *sc, UErrorCode &status);
    static const SpoofImpl *validateThis(const USpoofChecker *sc, UErrorCode &status);

    




  
    int32_t confusableLookup(UChar32 inChar, int32_t tableMask, UnicodeString &destBuf) const;

    
    void setAllowedLocales(const char *localesList, UErrorCode &status);
    const char * getAllowedLocales(UErrorCode &status);

    
    
    void addScriptChars(const char *locale, UnicodeSet *allowedChars, UErrorCode &status);


    
    static UChar32 ScanHex(const UChar *s, int32_t start, int32_t limit, UErrorCode &status);

    
    
    
    void wholeScriptCheck(
        const UnicodeString &text, ScriptSet *result, UErrorCode &status) const;
	    
    static UClassID U_EXPORT2 getStaticClassID(void);
    virtual UClassID getDynamicClassID(void) const;

    
    
    
    
    IdentifierInfo *getIdentifierInfo(UErrorCode &status) const; 
    void releaseIdentifierInfo(IdentifierInfo *idInfo) const;

    
    
    

    int32_t           fMagic;             
    int32_t           fChecks;            

    SpoofData        *fSpoofData;
    
    const UnicodeSet *fAllowedCharsSet;   
                                          

    const char       *fAllowedLocales;    
    URestrictionLevel fRestrictionLevel;  

    IdentifierInfo    *fCachedIdentifierInfo;    
};


















































#define USPOOF_SL_TABLE_FLAG (1<<24)
#define USPOOF_SA_TABLE_FLAG (1<<25)
#define USPOOF_ML_TABLE_FLAG (1<<26)
#define USPOOF_MA_TABLE_FLAG (1<<27)
#define USPOOF_KEY_MULTIPLE_VALUES (1<<28)
#define USPOOF_KEY_LENGTH_SHIFT 29
#define USPOOF_KEY_LENGTH_FIELD(x) (((x)>>29) & 3)


struct SpoofStringLengthsElement {
    uint16_t      fLastString;         
    uint16_t      fStrLength;           
};

















class SpoofData: public UMemory {
  public:
    static SpoofData *getDefault(UErrorCode &status);   
    SpoofData(UErrorCode &status);   
                                     
    
    
    
    
    SpoofData(UDataMemory *udm, UErrorCode &status);

    
    
    SpoofData(const void *serializedData, int32_t length, UErrorCode &status);

    
    
    static UBool validateDataVersion(const SpoofDataHeader *rawData, UErrorCode &status);
    ~SpoofData();                    
                                     
    
    
    
    
    SpoofData *addReference(); 
    void removeReference();

    
    
    
    
    
    void *reserveSpace(int32_t numBytes, UErrorCode &status);

    
    void initPtrs(UErrorCode &status);

    
    
    void reset();
    
    SpoofDataHeader             *fRawData;          
    UBool                       fDataOwned;         
                                                    
    UDataMemory                 *fUDM;              
                                                    
                                                    

    uint32_t                    fMemLimit;          
    u_atomic_int32_t            fRefCount;

    
    int32_t                     *fCFUKeys;
    uint16_t                    *fCFUValues;
    SpoofStringLengthsElement   *fCFUStringLengths;
    UChar                       *fCFUStrings;

    
    UTrie2                      *fAnyCaseTrie;
    UTrie2                      *fLowerCaseTrie;
    ScriptSet                   *fScriptSets;
    };
    







struct SpoofDataHeader {
    int32_t       fMagic;                
    uint8_t       fFormatVersion[4];     
                                         
    int32_t       fLength;               
                                         

    
    

    int32_t       fCFUKeys;               
    int32_t       fCFUKeysSize;           

    
    int32_t       fCFUStringIndex;        
    int32_t       fCFUStringIndexSize;    
                                          

    int32_t       fCFUStringTable;        
    int32_t       fCFUStringTableLen;     

    int32_t       fCFUStringLengths;      
    int32_t       fCFUStringLengthsSize;  


    
    
    int32_t       fAnyCaseTrie;           
    int32_t       fAnyCaseTrieLength;     
    
    int32_t       fLowerCaseTrie;         
    int32_t       fLowerCaseTrieLength;   

    int32_t       fScriptSets;            
    int32_t       fScriptSetsLength;      
    

    
    
    
    int32_t       unused[15];              
    
 }; 



    

















U_NAMESPACE_END
#endif 





U_CAPI int32_t U_EXPORT2
uspoof_swap(const UDataSwapper *ds, const void *inData, int32_t length, void *outData,
            UErrorCode *status);


#endif

#endif  

