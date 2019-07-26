





























#include "unicode/utypes.h"

#if !UCONFIG_NO_BREAK_ITERATION

#include "unicode/uniset.h"
#include "utrie.h"
#include "uvector.h"
#include "uassert.h"
#include "cmemory.h"
#include "cstring.h"

#include "rbbisetb.h"
#include "rbbinode.h"












U_CDECL_BEGIN
static uint32_t U_CALLCONV
getFoldedRBBIValue(UNewTrie *trie, UChar32 start, int32_t offset) {
    uint32_t value;
    UChar32 limit;
    UBool inBlockZero;

    limit=start+0x400;
    while(start<limit) {
        value=utrie_get32(trie, start, &inBlockZero);
        if(inBlockZero) {
            start+=UTRIE_DATA_BLOCK_LENGTH;
        } else if(value!=0) {
            return (uint32_t)(offset|0x8000);
        } else {
            ++start;
        }
    }
    return 0;
}


U_CDECL_END



U_NAMESPACE_BEGIN






RBBISetBuilder::RBBISetBuilder(RBBIRuleBuilder *rb)
{
    fRB             = rb;
    fStatus         = rb->fStatus;
    fRangeList      = 0;
    fTrie           = 0;
    fTrieSize       = 0;
    fGroupCount     = 0;
    fSawBOF         = FALSE;
}







RBBISetBuilder::~RBBISetBuilder()
{
    RangeDescriptor   *nextRangeDesc;

    
    for (nextRangeDesc = fRangeList; nextRangeDesc!=NULL;) {
        RangeDescriptor *r = nextRangeDesc;
        nextRangeDesc      = r->fNext;
        delete r;
    }

    utrie_close(fTrie);
}










void RBBISetBuilder::build() {
    RBBINode        *usetNode;
    RangeDescriptor *rlRange;

    if (fRB->fDebugEnv && uprv_strstr(fRB->fDebugEnv, "usets")) {printSets();}

    
    
    
    
    fRangeList                = new RangeDescriptor(*fStatus); 
    if (fRangeList == NULL) {
        *fStatus = U_MEMORY_ALLOCATION_ERROR;
        return;
    }
    fRangeList->fStartChar    = 0;
    fRangeList->fEndChar      = 0x10ffff;

    if (U_FAILURE(*fStatus)) {
        return;
    }

    
    
    
    int  ni;
    for (ni=0; ; ni++) {        
        usetNode = (RBBINode *)this->fRB->fUSetNodes->elementAt(ni);
        if (usetNode==NULL) {
            break;
        }

        UnicodeSet      *inputSet             = usetNode->fInputSet;
        int32_t          inputSetRangeCount   = inputSet->getRangeCount();
        int              inputSetRangeIndex   = 0;
                         rlRange              = fRangeList;

        for (;;) {
            if (inputSetRangeIndex >= inputSetRangeCount) {
                break;
            }
            UChar32      inputSetRangeBegin  = inputSet->getRangeStart(inputSetRangeIndex);
            UChar32      inputSetRangeEnd    = inputSet->getRangeEnd(inputSetRangeIndex);

            
            
            while (rlRange->fEndChar < inputSetRangeBegin) {
                rlRange = rlRange->fNext;
            }

            
            
            
            
            
            
            if (rlRange->fStartChar < inputSetRangeBegin) {
                rlRange->split(inputSetRangeBegin, *fStatus);
                if (U_FAILURE(*fStatus)) {
                    return;
                }
                continue;
            }

            
            
            
            
            
            if (rlRange->fEndChar > inputSetRangeEnd) {
                rlRange->split(inputSetRangeEnd+1, *fStatus);
                if (U_FAILURE(*fStatus)) {
                    return;
                }
            }

            
            
            if (rlRange->fIncludesSets->indexOf(usetNode) == -1) {
                rlRange->fIncludesSets->addElement(usetNode, *fStatus);
                if (U_FAILURE(*fStatus)) {
                    return;
                }
            }

            
            if (inputSetRangeEnd == rlRange->fEndChar) {
                inputSetRangeIndex++;
            }
            rlRange = rlRange->fNext;
        }
    }

    if (fRB->fDebugEnv && uprv_strstr(fRB->fDebugEnv, "range")) { printRanges();}

    
    
    
    
    
    
    
    
    
    
    
    RangeDescriptor *rlSearchRange;
    for (rlRange = fRangeList; rlRange!=0; rlRange=rlRange->fNext) {
        for (rlSearchRange=fRangeList; rlSearchRange != rlRange; rlSearchRange=rlSearchRange->fNext) {
            if (rlRange->fIncludesSets->equals(*rlSearchRange->fIncludesSets)) {
                rlRange->fNum = rlSearchRange->fNum;
                break;
            }
        }
        if (rlRange->fNum == 0) {
            fGroupCount ++;
            rlRange->fNum = fGroupCount+2; 
            rlRange->setDictionaryFlag();
            addValToSets(rlRange->fIncludesSets, fGroupCount+2);
        }
    }

    
    
    
    
    
    
    
    
    
    static const UChar eofUString[] = {0x65, 0x6f, 0x66, 0};
    static const UChar bofUString[] = {0x62, 0x6f, 0x66, 0};

    UnicodeString eofString(eofUString);
    UnicodeString bofString(bofUString);
    for (ni=0; ; ni++) {        
        usetNode = (RBBINode *)this->fRB->fUSetNodes->elementAt(ni);
        if (usetNode==NULL) {
            break;
        }
        UnicodeSet      *inputSet = usetNode->fInputSet;
        if (inputSet->contains(eofString)) {
            addValToSet(usetNode, 1);
        }
        if (inputSet->contains(bofString)) {
            addValToSet(usetNode, 2);
            fSawBOF = TRUE;
        }
    }


    if (fRB->fDebugEnv && uprv_strstr(fRB->fDebugEnv, "rgroup")) {printRangeGroups();}
    if (fRB->fDebugEnv && uprv_strstr(fRB->fDebugEnv, "esets")) {printSets();}

    
    
    
    
    fTrie = utrie_open(NULL,    
                      NULL,    
                      100000,  
                      0,       
                      0,       
                      TRUE);   


    for (rlRange = fRangeList; rlRange!=0; rlRange=rlRange->fNext) {
        utrie_setRange32(fTrie, rlRange->fStartChar, rlRange->fEndChar+1, rlRange->fNum, TRUE);
    }
}








int32_t RBBISetBuilder::getTrieSize()  {
    fTrieSize  = utrie_serialize(fTrie,
                                    NULL,                
                                    0,                   
                                    getFoldedRBBIValue,
                                    TRUE,                
                                    fStatus);
    
    return fTrieSize;
}









void RBBISetBuilder::serializeTrie(uint8_t *where) {
    utrie_serialize(fTrie,
                    where,                   
                    fTrieSize,               
                    getFoldedRBBIValue,
                    TRUE,                    
                    fStatus);
}















void  RBBISetBuilder::addValToSets(UVector *sets, uint32_t val) {
    int32_t       ix;

    for (ix=0; ix<sets->size(); ix++) {
        RBBINode *usetNode = (RBBINode *)sets->elementAt(ix);
        addValToSet(usetNode, val);
    }
}

void  RBBISetBuilder::addValToSet(RBBINode *usetNode, uint32_t val) {
    RBBINode *leafNode = new RBBINode(RBBINode::leafChar);
    if (leafNode == NULL) {
        *fStatus = U_MEMORY_ALLOCATION_ERROR;
        return;
    }
    leafNode->fVal = (unsigned short)val;
    if (usetNode->fLeftChild == NULL) {
        usetNode->fLeftChild = leafNode;
        leafNode->fParent    = usetNode;
    } else {
        
        
        
        RBBINode *orNode = new RBBINode(RBBINode::opOr);
        if (orNode == NULL) {
            *fStatus = U_MEMORY_ALLOCATION_ERROR;
            return;
        }
        orNode->fLeftChild  = usetNode->fLeftChild;
        orNode->fRightChild = leafNode;
        orNode->fLeftChild->fParent  = orNode;
        orNode->fRightChild->fParent = orNode;
        usetNode->fLeftChild = orNode;
        orNode->fParent = usetNode;
    }
}







int32_t  RBBISetBuilder::getNumCharCategories() const {
    return fGroupCount + 3;
}







UBool  RBBISetBuilder::sawBOF() const {
    return fSawBOF;
}








UChar32  RBBISetBuilder::getFirstChar(int32_t category) const {
    RangeDescriptor   *rlRange;
    UChar32            retVal = (UChar32)-1;
    for (rlRange = fRangeList; rlRange!=0; rlRange=rlRange->fNext) {
        if (rlRange->fNum == category) {
            retVal = rlRange->fStartChar;
            break;
        }
    }
    return retVal;
}









#ifdef RBBI_DEBUG
void RBBISetBuilder::printRanges() {
    RangeDescriptor       *rlRange;
    int                    i;

    RBBIDebugPrintf("\n\n Nonoverlapping Ranges ...\n");
    for (rlRange = fRangeList; rlRange!=0; rlRange=rlRange->fNext) {
        RBBIDebugPrintf("%2i  %4x-%4x  ", rlRange->fNum, rlRange->fStartChar, rlRange->fEndChar);

        for (i=0; i<rlRange->fIncludesSets->size(); i++) {
            RBBINode       *usetNode    = (RBBINode *)rlRange->fIncludesSets->elementAt(i);
            UnicodeString   setName = UNICODE_STRING("anon", 4);
            RBBINode       *setRef = usetNode->fParent;
            if (setRef != NULL) {
                RBBINode *varRef = setRef->fParent;
                if (varRef != NULL  &&  varRef->fType == RBBINode::varRef) {
                    setName = varRef->fText;
                }
            }
            RBBI_DEBUG_printUnicodeString(setName); RBBIDebugPrintf("  ");
        }
        RBBIDebugPrintf("\n");
    }
}
#endif








#ifdef RBBI_DEBUG
void RBBISetBuilder::printRangeGroups() {
    RangeDescriptor       *rlRange;
    RangeDescriptor       *tRange;
    int                    i;
    int                    lastPrintedGroupNum = 0;

    RBBIDebugPrintf("\nRanges grouped by Unicode Set Membership...\n");
    for (rlRange = fRangeList; rlRange!=0; rlRange=rlRange->fNext) {
        int groupNum = rlRange->fNum & 0xbfff;
        if (groupNum > lastPrintedGroupNum) {
            lastPrintedGroupNum = groupNum;
            RBBIDebugPrintf("%2i  ", groupNum);

            if (rlRange->fNum & 0x4000) { RBBIDebugPrintf(" <DICT> ");}

            for (i=0; i<rlRange->fIncludesSets->size(); i++) {
                RBBINode       *usetNode    = (RBBINode *)rlRange->fIncludesSets->elementAt(i);
                UnicodeString   setName = UNICODE_STRING("anon", 4);
                RBBINode       *setRef = usetNode->fParent;
                if (setRef != NULL) {
                    RBBINode *varRef = setRef->fParent;
                    if (varRef != NULL  &&  varRef->fType == RBBINode::varRef) {
                        setName = varRef->fText;
                    }
                }
                RBBI_DEBUG_printUnicodeString(setName); RBBIDebugPrintf(" ");
            }

            i = 0;
            for (tRange = rlRange; tRange != 0; tRange = tRange->fNext) {
                if (tRange->fNum == rlRange->fNum) {
                    if (i++ % 5 == 0) {
                        RBBIDebugPrintf("\n    ");
                    }
                    RBBIDebugPrintf("  %05x-%05x", tRange->fStartChar, tRange->fEndChar);
                }
            }
            RBBIDebugPrintf("\n");
        }
    }
    RBBIDebugPrintf("\n");
}
#endif








#ifdef RBBI_DEBUG
void RBBISetBuilder::printSets() {
    int                   i;

    RBBIDebugPrintf("\n\nUnicode Sets List\n------------------\n");
    for (i=0; ; i++) {
        RBBINode        *usetNode;
        RBBINode        *setRef;
        RBBINode        *varRef;
        UnicodeString    setName;

        usetNode = (RBBINode *)fRB->fUSetNodes->elementAt(i);
        if (usetNode == NULL) {
            break;
        }

        RBBIDebugPrintf("%3d    ", i);
        setName = UNICODE_STRING("anonymous", 9);
        setRef = usetNode->fParent;
        if (setRef != NULL) {
            varRef = setRef->fParent;
            if (varRef != NULL  &&  varRef->fType == RBBINode::varRef) {
                setName = varRef->fText;
            }
        }
        RBBI_DEBUG_printUnicodeString(setName);
        RBBIDebugPrintf("   ");
        RBBI_DEBUG_printUnicodeString(usetNode->fText);
        RBBIDebugPrintf("\n");
        if (usetNode->fLeftChild != NULL) {
            usetNode->fLeftChild->printTree(TRUE);
        }
    }
    RBBIDebugPrintf("\n");
}
#endif









RangeDescriptor::RangeDescriptor(const RangeDescriptor &other, UErrorCode &status) {
    int  i;

    this->fStartChar    = other.fStartChar;
    this->fEndChar      = other.fEndChar;
    this->fNum          = other.fNum;
    this->fNext         = NULL;
    UErrorCode oldstatus = status;
    this->fIncludesSets = new UVector(status);
    if (U_FAILURE(oldstatus)) {
        status = oldstatus;
    }
    if (U_FAILURE(status)) {
        return;
    }
    
    if (this->fIncludesSets == 0) {
        status = U_MEMORY_ALLOCATION_ERROR;
        return;
    }

    for (i=0; i<other.fIncludesSets->size(); i++) {
        this->fIncludesSets->addElement(other.fIncludesSets->elementAt(i), status);
    }
}







RangeDescriptor::RangeDescriptor(UErrorCode &status) {
    this->fStartChar    = 0;
    this->fEndChar      = 0;
    this->fNum          = 0;
    this->fNext         = NULL;
    UErrorCode oldstatus = status;
    this->fIncludesSets = new UVector(status);
    if (U_FAILURE(oldstatus)) {
        status = oldstatus;
    }
    if (U_FAILURE(status)) {
        return;
    }
    
    if(this->fIncludesSets == 0) {
        status = U_MEMORY_ALLOCATION_ERROR;
        return;
    }

}







RangeDescriptor::~RangeDescriptor() {
    delete  fIncludesSets;
    fIncludesSets = NULL;
}






void RangeDescriptor::split(UChar32 where, UErrorCode &status) {
    U_ASSERT(where>fStartChar && where<=fEndChar);
    RangeDescriptor *nr = new RangeDescriptor(*this, status);
    if(nr == 0) {
        status = U_MEMORY_ALLOCATION_ERROR;
        return;
    }
    if (U_FAILURE(status)) {
        delete nr;
        return;
    }
    
    
    nr->fStartChar = where;
    this->fEndChar = where-1;
    nr->fNext      = this->fNext;
    this->fNext    = nr;
}




















void RangeDescriptor::setDictionaryFlag() {
    int i;

    for (i=0; i<this->fIncludesSets->size(); i++) {
        RBBINode       *usetNode    = (RBBINode *)fIncludesSets->elementAt(i);
        UnicodeString   setName;
        RBBINode       *setRef = usetNode->fParent;
        if (setRef != NULL) {
            RBBINode *varRef = setRef->fParent;
            if (varRef != NULL  &&  varRef->fType == RBBINode::varRef) {
                setName = varRef->fText;
            }
        }
        if (setName.compare(UNICODE_STRING("dictionary", 10)) == 0) {   
            this->fNum |= 0x4000;
            break;
        }
    }
}



U_NAMESPACE_END

#endif 
