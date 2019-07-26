






























#ifndef __RBBIDATA_H__
#define __RBBIDATA_H__

#include "unicode/utypes.h"
#include "unicode/udata.h"
#include "udataswp.h"





U_CAPI int32_t U_EXPORT2
ubrk_swap(const UDataSwapper *ds,
          const void *inData, int32_t length, void *outData,
          UErrorCode *pErrorCode);

#ifdef __cplusplus

#include "unicode/uobject.h"
#include "unicode/unistr.h"
#include "utrie.h"

U_NAMESPACE_BEGIN




struct RBBIDataHeader {
    uint32_t         fMagic;           
    uint8_t          fFormatVersion[4]; 
                                       
                                       
                                       
                                       
                                       
    uint32_t         fLength;          
                                       
    uint32_t         fCatCount;        

    
    
    
    
    
    uint32_t         fFTable;         
    uint32_t         fFTableLen;
    uint32_t         fRTable;         
    uint32_t         fRTableLen;
    uint32_t         fSFTable;        
    uint32_t         fSFTableLen;
    uint32_t         fSRTable;        
    uint32_t         fSRTableLen;
    uint32_t         fTrie;           
    uint32_t         fTrieLen;
    uint32_t         fRuleSource;     
    uint32_t         fRuleSourceLen;  
    uint32_t         fStatusTable;    
    uint32_t         fStatusTableLen;

    uint32_t         fReserved[6];    

};



struct  RBBIStateTableRow {
    int16_t          fAccepting;    
                                    
                                    
                                    
                                    
                                    
                                    
    int16_t          fLookAhead;    
                                    
                                    
                                    
                                    
    int16_t          fTagIdx;       
                                    
                                    
                                    
    int16_t          fReserved;
    uint16_t         fNextState[2]; 
                                    
                                    
                                    
                                    
};


struct RBBIStateTable {
    uint32_t         fNumStates;    
    uint32_t         fRowLen;       
    uint32_t         fFlags;        
    uint32_t         fReserved;     
    char             fTableData[4]; 
                                    
                                    
};

typedef enum {
    RBBI_LOOKAHEAD_HARD_BREAK = 1,
    RBBI_BOF_REQUIRED = 2
} RBBIStateTableFlags;





class RBBIDataWrapper : public UMemory {
public:
    enum EDontAdopt {
        kDontAdopt
    };
    RBBIDataWrapper(const RBBIDataHeader *data, UErrorCode &status);
    RBBIDataWrapper(const RBBIDataHeader *data, enum EDontAdopt dontAdopt, UErrorCode &status);
    RBBIDataWrapper(UDataMemory* udm, UErrorCode &status);
    ~RBBIDataWrapper();

    void                  init(const RBBIDataHeader *data, UErrorCode &status);
    RBBIDataWrapper      *addReference();
    void                  removeReference();
    UBool                 operator ==(const RBBIDataWrapper &other) const;
    int32_t               hashCode();
    const UnicodeString  &getRuleSourceString() const;
#ifdef RBBI_DEBUG
    void                  printData();
    void                  printTable(const char *heading, const RBBIStateTable *table);
#else
    #define printData()
    #define printTable(heading, table)
#endif

    
    
    
    const RBBIDataHeader     *fHeader;
    const RBBIStateTable     *fForwardTable;
    const RBBIStateTable     *fReverseTable;
    const RBBIStateTable     *fSafeFwdTable;
    const RBBIStateTable     *fSafeRevTable;
    const UChar              *fRuleSource;
    const int32_t            *fRuleStatusTable; 

    
    int32_t             fStatusMaxIdx;

    UTrie               fTrie;

private:
    int32_t             fRefCount;
    UDataMemory        *fUDataMem;
    UnicodeString       fRuleString;
    UBool               fDontFreeData;

    RBBIDataWrapper(const RBBIDataWrapper &other); 
    RBBIDataWrapper &operator=(const RBBIDataWrapper &other); 
};



U_NAMESPACE_END

#endif

#endif
