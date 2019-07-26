









#include "unicode/utypes.h"

#if !UCONFIG_NO_BREAK_ITERATION

#include "unicode/unistr.h"
#include "unicode/uniset.h"
#include "unicode/uchar.h"
#include "unicode/parsepos.h"

#include "umutex.h"

#include "rbbirb.h"
#include "rbbinode.h"






U_CDECL_BEGIN
static void U_CALLCONV RBBISymbolTableEntry_deleter(void *p) {
    icu::RBBISymbolTableEntry *px = (icu::RBBISymbolTableEntry *)p;
    delete px;
}
U_CDECL_END



U_NAMESPACE_BEGIN

RBBISymbolTable::RBBISymbolTable(RBBIRuleScanner *rs, const UnicodeString &rules, UErrorCode &status)
    :fRules(rules), fRuleScanner(rs), ffffString(UChar(0xffff))
{
    fHashTable       = NULL;
    fCachedSetLookup = NULL;
    
    fHashTable = uhash_open(uhash_hashUnicodeString, uhash_compareUnicodeString, NULL, &status);
    
    if (U_FAILURE(status)) {
        return;
    }
    uhash_setValueDeleter(fHashTable, RBBISymbolTableEntry_deleter);
}



RBBISymbolTable::~RBBISymbolTable()
{
    uhash_close(fHashTable);
}









const UnicodeString  *RBBISymbolTable::lookup(const UnicodeString& s) const
{
    RBBISymbolTableEntry  *el;
    RBBINode              *varRefNode;
    RBBINode              *exprNode;
    RBBINode              *usetNode;
    const UnicodeString   *retString;
    RBBISymbolTable       *This = (RBBISymbolTable *)this;   

    el = (RBBISymbolTableEntry *)uhash_get(fHashTable, &s);
    if (el == NULL) {
        return NULL;
    }

    varRefNode = el->val;
    exprNode   = varRefNode->fLeftChild;     
    if (exprNode->fType == RBBINode::setRef) {
        
        
        
        usetNode = exprNode->fLeftChild;
        This->fCachedSetLookup = usetNode->fInputSet;
        retString = &ffffString;
    }
    else
    {
        
        
        retString = &exprNode->fText;
        This->fCachedSetLookup = NULL;
    }
    return retString;
}














const UnicodeFunctor *RBBISymbolTable::lookupMatcher(UChar32 ch) const
{
    UnicodeSet *retVal = NULL;
    RBBISymbolTable *This = (RBBISymbolTable *)this;   
    if (ch == 0xffff) {
        retVal = fCachedSetLookup;
        This->fCachedSetLookup = 0;
    }
    return retVal;
}











UnicodeString   RBBISymbolTable::parseReference(const UnicodeString& text,
                                                ParsePosition& pos, int32_t limit) const
{
    int32_t start = pos.getIndex();
    int32_t i = start;
    UnicodeString result;
    while (i < limit) {
        UChar c = text.charAt(i);
        if ((i==start && !u_isIDStart(c)) || !u_isIDPart(c)) {
            break;
        }
        ++i;
    }
    if (i == start) { 
        return result; 
    }
    pos.setIndex(i);
    text.extractBetween(start, i, result);
    return result;
}








RBBINode       *RBBISymbolTable::lookupNode(const UnicodeString &key) const{

    RBBINode             *retNode = NULL;
    RBBISymbolTableEntry *el;

    el = (RBBISymbolTableEntry *)uhash_get(fHashTable, &key);
    if (el != NULL) {
        retNode = el->val;
    }
    return retNode;
}








void            RBBISymbolTable::addEntry  (const UnicodeString &key, RBBINode *val, UErrorCode &err) {
    RBBISymbolTableEntry *e;
    
    if (U_FAILURE(err)) {
        return;
    }
    e = (RBBISymbolTableEntry *)uhash_get(fHashTable, &key);
    if (e != NULL) {
        err = U_BRK_VARIABLE_REDFINITION;
        return;
    }

    e = new RBBISymbolTableEntry;
    if (e == NULL) {
        err = U_MEMORY_ALLOCATION_ERROR;
        return;
    }
    e->key = key;
    e->val = val;
    uhash_put( fHashTable, &e->key, e, &err);
}


RBBISymbolTableEntry::RBBISymbolTableEntry() : UMemory(), key(), val(NULL) {}

RBBISymbolTableEntry::~RBBISymbolTableEntry() {
    
    
    
    
    delete val->fLeftChild;
    val->fLeftChild = NULL;

    delete  val;

    
}





#ifdef RBBI_DEBUG
void RBBISymbolTable::rbbiSymtablePrint() const {
    RBBIDebugPrintf("Variable Definitions\n"
           "Name               Node Val     String Val\n"
           "----------------------------------------------------------------------\n");

    int32_t pos = -1;
    const UHashElement  *e   = NULL;
    for (;;) {
        e = uhash_nextElement(fHashTable,  &pos);
        if (e == NULL ) {
            break;
        }
        RBBISymbolTableEntry  *s   = (RBBISymbolTableEntry *)e->value.pointer;

        RBBI_DEBUG_printUnicodeString(s->key, 15);
        RBBIDebugPrintf("   %8p   ", (void *)s->val);
        RBBI_DEBUG_printUnicodeString(s->val->fLeftChild->fText);
        RBBIDebugPrintf("\n");
    }

    RBBIDebugPrintf("\nParsed Variable Definitions\n");
    pos = -1;
    for (;;) {
        e = uhash_nextElement(fHashTable,  &pos);
        if (e == NULL ) {
            break;
        }
        RBBISymbolTableEntry  *s   = (RBBISymbolTableEntry *)e->value.pointer;
        RBBI_DEBUG_printUnicodeString(s->key);
        s->val->fLeftChild->printTree(TRUE);
        RBBIDebugPrintf("\n");
    }
}
#endif





U_NAMESPACE_END

#endif 
