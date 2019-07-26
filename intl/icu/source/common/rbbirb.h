










#ifndef RBBIRB_H
#define RBBIRB_H

#include "unicode/utypes.h"
#include "unicode/uobject.h"
#include "unicode/rbbi.h"
#include "unicode/uniset.h"
#include "unicode/parseerr.h"
#include "uhash.h"
#include "uvector.h"
#include "unicode/symtable.h"
                          



U_NAMESPACE_BEGIN

class               RBBIRuleScanner;
struct              RBBIRuleTableEl;
class               RBBISetBuilder;
class               RBBINode;
class               RBBITableBuilder;









class RBBISymbolTableEntry : public UMemory { 
public:                                       
    RBBISymbolTableEntry();
    UnicodeString          key;
    RBBINode               *val;
    ~RBBISymbolTableEntry();

private:
    RBBISymbolTableEntry(const RBBISymbolTableEntry &other); 
    RBBISymbolTableEntry &operator=(const RBBISymbolTableEntry &other); 
};


class RBBISymbolTable : public UMemory, public SymbolTable {
private:
    const UnicodeString      &fRules;
    UHashtable               *fHashTable;
    RBBIRuleScanner          *fRuleScanner;

    
    
    
    const UnicodeString      ffffString;      
    UnicodeSet              *fCachedSetLookup;

public:
    
    virtual const UnicodeString*  lookup(const UnicodeString& s) const;
    virtual const UnicodeFunctor* lookupMatcher(UChar32 ch) const;
    virtual UnicodeString parseReference(const UnicodeString& text,
                                         ParsePosition& pos, int32_t limit) const;

    
    RBBISymbolTable(RBBIRuleScanner *, const UnicodeString &fRules, UErrorCode &status);
    virtual ~RBBISymbolTable();

    virtual RBBINode *lookupNode(const UnicodeString &key) const;
    virtual void      addEntry  (const UnicodeString &key, RBBINode *val, UErrorCode &err);

#ifdef RBBI_DEBUG
    virtual void      rbbiSymtablePrint() const;
#else
    
    
    int32_t fFakeField;
    #define rbbiSymtablePrint() fFakeField=0; 
#endif

private:
    RBBISymbolTable(const RBBISymbolTable &other); 
    RBBISymbolTable &operator=(const RBBISymbolTable &other); 
};







class RBBIRuleBuilder : public UMemory {
public:

    
    
    
    
    static BreakIterator * createRuleBasedBreakIterator( const UnicodeString    &rules,
                                    UParseError      *parseError,
                                    UErrorCode       &status);

public:
    
    
    
    
    RBBIRuleBuilder(const UnicodeString  &rules,
                    UParseError          *parseErr,
                    UErrorCode           &status
        );

    virtual    ~RBBIRuleBuilder();
    char                          *fDebugEnv;        
    UErrorCode                    *fStatus;          
    UParseError                   *fParseError;      
    const UnicodeString           &fRules;           

    RBBIRuleScanner               *fScanner;         
    RBBINode                      *fForwardTree;     
    RBBINode                      *fReverseTree;     
    RBBINode                      *fSafeFwdTree;
    RBBINode                      *fSafeRevTree;

    RBBINode                      **fDefaultTree;    
                                                     

    UBool                         fChainRules;       
                                                     

    UBool                         fLBCMNoChain;      
                                                     

    UBool                         fLookAheadHardBreak;  
                                                     
                                                     

    RBBISetBuilder                *fSetBuilder;      
    UVector                       *fUSetNodes;       

    RBBITableBuilder              *fForwardTables;   
    RBBITableBuilder              *fReverseTables;
    RBBITableBuilder              *fSafeFwdTables;
    RBBITableBuilder              *fSafeRevTables;

    UVector                       *fRuleStatusVals;  
                                                     

    RBBIDataHeader                *flattenData();    
                                                     
private:
    RBBIRuleBuilder(const RBBIRuleBuilder &other); 
    RBBIRuleBuilder &operator=(const RBBIRuleBuilder &other); 
};



















struct RBBISetTableEl {
    UnicodeString *key;
    RBBINode      *val;
};










#ifdef RBBI_DEBUG
#include <stdio.h>
#define RBBIDebugPrintf printf
#define RBBIDebugPuts puts
#else
#undef RBBIDebugPrintf 
#define RBBIDebugPuts(arg)
#endif

U_NAMESPACE_END
#endif



