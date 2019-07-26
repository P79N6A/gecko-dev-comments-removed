










#ifndef RBBITBLB_H
#define RBBITBLB_H

#include "unicode/utypes.h"
#include "unicode/uobject.h"
#include "unicode/rbbi.h"
#include "rbbinode.h"


U_NAMESPACE_BEGIN

class RBBIRuleScanner;
class RBBIRuleBuilder;










class RBBITableBuilder : public UMemory {
public:
    RBBITableBuilder(RBBIRuleBuilder *rb, RBBINode **rootNode);
    ~RBBITableBuilder();

    void     build();
    int32_t  getTableSize() const;      
                                        
    void     exportTable(void *where);  
                                        
                                        


private:
    void     calcNullable(RBBINode *n);
    void     calcFirstPos(RBBINode *n);
    void     calcLastPos(RBBINode  *n);
    void     calcFollowPos(RBBINode *n);
    void     calcChainedFollowPos(RBBINode *n);
    void     bofFixup();
    void     buildStateTable();
    void     flagAcceptingStates();
    void     flagLookAheadStates();
    void     flagTaggedStates();
    void     mergeRuleStatusVals();

    
    

    void     setAdd(UVector *dest, UVector *source);
    UBool    setEquals(UVector *a, UVector *b);

    void     sortedAdd(UVector **dest, int32_t val);

public:
#ifdef RBBI_DEBUG
    void     printSet(UVector *s);
    void     printPosSets(RBBINode *n );
    void     printStates();
    void     printRuleStatusTable();
#else
    #define  printSet(s)
    #define  printPosSets(n)
    #define  printStates()
    #define  printRuleStatusTable()
#endif

private:
    RBBIRuleBuilder  *fRB;
    RBBINode         *&fTree;              
                                           
    UErrorCode       *fStatus;

    UVector          *fDStates;            
                                           
                                           


    RBBITableBuilder(const RBBITableBuilder &other); 
    RBBITableBuilder &operator=(const RBBITableBuilder &other); 
};




class RBBIStateDescriptor : public UMemory {
public:
    UBool            fMarked;
    int32_t          fAccepting;
    int32_t          fLookAhead;
    UVector          *fTagVals;
    int32_t          fTagsIdx;
    UVector          *fPositions;          
                                           
                                           

    UVector          *fDtran;              
                                           
                                           
                                           

    RBBIStateDescriptor(int maxInputSymbol,  UErrorCode *fStatus);
    ~RBBIStateDescriptor();

private:
    RBBIStateDescriptor(const RBBIStateDescriptor &other); 
    RBBIStateDescriptor &operator=(const RBBIStateDescriptor &other); 
};



U_NAMESPACE_END
#endif
