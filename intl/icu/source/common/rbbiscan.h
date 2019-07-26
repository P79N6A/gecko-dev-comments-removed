









#ifndef RBBISCAN_H
#define RBBISCAN_H

#include "unicode/utypes.h"
#include "unicode/uobject.h"
#include "unicode/rbbi.h"
#include "unicode/uniset.h"
#include "unicode/parseerr.h"
#include "uhash.h"
#include "uvector.h"
#include "unicode/symtable.h"
                          
#include "rbbinode.h"




U_NAMESPACE_BEGIN

class   RBBIRuleBuilder;
class   RBBISymbolTable;













class RBBIRuleScanner : public UMemory {
public:

    enum {
        kStackSize = 100            
    };                              
                                    
                                    

    struct RBBIRuleChar {
        UChar32             fChar;
        UBool               fEscaped;
    };

    RBBIRuleScanner(RBBIRuleBuilder  *rb);


    virtual    ~RBBIRuleScanner();

    void        nextChar(RBBIRuleChar &c);          
                                                    

    UBool       push(const RBBIRuleChar &c);        
                                                    

    void        parse();                            
                                                    
                                                    
                                                    

    



    static UnicodeString stripRules(const UnicodeString &rules);
private:

    UBool       doParseActions(int32_t a);
    void        error(UErrorCode e);                   
    void        fixOpStack(RBBINode::OpPrecedence p);
                                                       
    void        findSetFor(const UnicodeString &s, RBBINode *node, UnicodeSet *setToAdopt = NULL);

    UChar32     nextCharLL();
#ifdef RBBI_DEBUG
    void        printNodeStack(const char *title);
#endif
    RBBINode    *pushNewNode(RBBINode::NodeType  t);
    void        scanSet();


    RBBIRuleBuilder               *fRB;              

    int32_t                       fScanIndex;        
                                                     
    int32_t                       fNextIndex;        
                                                     
    UBool                         fQuoteMode;        
    int32_t                       fLineNum;          
    int32_t                       fCharNum;          
    UChar32                       fLastChar;         
                                                     

    RBBIRuleChar                  fC;                
                                                     
    UnicodeString                 fVarName;          
                                                     

    RBBIRuleTableEl               **fStateTable;     
                                                     

    uint16_t                      fStack[kStackSize];  
    int32_t                       fStackPtr;           
                                                       

    RBBINode                      *fNodeStack[kStackSize]; 
                                                           
    int32_t                        fNodeStackPtr;


    UBool                          fReverseRule;     
                                                     
                                                     

    UBool                          fLookAheadRule;   
                                                     

    RBBISymbolTable               *fSymbolTable;     
                                                     

    UHashtable                    *fSetTable;        
                                                     
                                                     
                                                     

    UnicodeSet                     fRuleSets[10];    
                                                     
                                                     
                                                     
                                                     

    int32_t                        fRuleNum;         

    int32_t                        fOptionStart;     
                                                     

    UnicodeSet *gRuleSet_rule_char;
    UnicodeSet *gRuleSet_white_space;
    UnicodeSet *gRuleSet_name_char;
    UnicodeSet *gRuleSet_name_start_char;

    RBBIRuleScanner(const RBBIRuleScanner &other); 
    RBBIRuleScanner &operator=(const RBBIRuleScanner &other); 
};

U_NAMESPACE_END

#endif
