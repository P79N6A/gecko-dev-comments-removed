








#ifndef RBBISETB_H
#define RBBISETB_H

#include "unicode/utypes.h"
#include "unicode/uobject.h"
#include "rbbirb.h"
#include "uvector.h"

struct  UNewTrie;

U_NAMESPACE_BEGIN
















class RangeDescriptor : public UMemory {
public:
    UChar32            fStartChar;      
    UChar32            fEndChar;        
    int32_t            fNum;            
    UVector           *fIncludesSets;   
                                        
                                        
    RangeDescriptor   *fNext;           

    RangeDescriptor(UErrorCode &status);
    RangeDescriptor(const RangeDescriptor &other, UErrorCode &status);
    ~RangeDescriptor();
    void split(UChar32 where, UErrorCode &status);   
                                        
    void setDictionaryFlag();           
                                        

private:
    RangeDescriptor(const RangeDescriptor &other); 
    RangeDescriptor &operator=(const RangeDescriptor &other); 
};

















class RBBISetBuilder : public UMemory {
public:
    RBBISetBuilder(RBBIRuleBuilder *rb);
    ~RBBISetBuilder();

    void     build();
    void     addValToSets(UVector *sets,      uint32_t val);
    void     addValToSet (RBBINode *usetNode, uint32_t val);
    int32_t  getNumCharCategories() const;   
                                             
                                             
    int32_t  getTrieSize() ;        
    void     serializeTrie(uint8_t *where);  
    UChar32  getFirstChar(int32_t  val) const;
    UBool    sawBOF() const;                 
                                             
#ifdef RBBI_DEBUG
    void     printSets();
    void     printRanges();
    void     printRangeGroups();
#else
    #define printSets()
    #define printRanges()
    #define printRangeGroups()
#endif

private:
    void           numberSets();

    RBBIRuleBuilder       *fRB;             
    UErrorCode            *fStatus;

    RangeDescriptor       *fRangeList;      

    UNewTrie              *fTrie;           
    uint32_t              fTrieSize;        

    
    
    
    
    
    
    int32_t               fGroupCount;

    UBool                 fSawBOF;

    RBBISetBuilder(const RBBISetBuilder &other); 
    RBBISetBuilder &operator=(const RBBISetBuilder &other); 
};



U_NAMESPACE_END
#endif
