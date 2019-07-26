










#include "unicode/utypes.h"

#if !UCONFIG_NO_BREAK_ITERATION

#include "unicode/unistr.h"
#include "rbbitblb.h"
#include "rbbirb.h"
#include "rbbisetb.h"
#include "rbbidata.h"
#include "cstring.h"
#include "uassert.h"
#include "cmemory.h"

U_NAMESPACE_BEGIN

RBBITableBuilder::RBBITableBuilder(RBBIRuleBuilder *rb, RBBINode **rootNode) :
 fTree(*rootNode) {
    fRB                 = rb;
    fStatus             = fRB->fStatus;
    UErrorCode status   = U_ZERO_ERROR;
    fDStates            = new UVector(status);
    if (U_FAILURE(*fStatus)) {
        return;
    }
    if (U_FAILURE(status)) {
        *fStatus = status;
        return;
    }
    if (fDStates == NULL) {
        *fStatus = U_MEMORY_ALLOCATION_ERROR;;
    }
}



RBBITableBuilder::~RBBITableBuilder() {
    int i;
    for (i=0; i<fDStates->size(); i++) {
        delete (RBBIStateDescriptor *)fDStates->elementAt(i);
    }
    delete   fDStates;
}








void  RBBITableBuilder::build() {

    if (U_FAILURE(*fStatus)) {
        return;
    }

    
    
    if (fTree==NULL) {
        return;
    }

    
    
    
    
    fTree = fTree->flattenVariables();
#ifdef RBBI_DEBUG
    if (fRB->fDebugEnv && uprv_strstr(fRB->fDebugEnv, "ftree")) {
        RBBIDebugPuts("Parse tree after flattening variable references.");
        fTree->printTree(TRUE);
    }
#endif

    
    
    
    
    
    
    if (fRB->fSetBuilder->sawBOF()) {
        RBBINode *bofTop    = new RBBINode(RBBINode::opCat);
        RBBINode *bofLeaf   = new RBBINode(RBBINode::leafChar);
        
        if (bofTop == NULL || bofLeaf == NULL) {
            *fStatus = U_MEMORY_ALLOCATION_ERROR;
            delete bofTop;
            delete bofLeaf;
            return;
        }
        bofTop->fLeftChild  = bofLeaf;
        bofTop->fRightChild = fTree;
        bofLeaf->fParent    = bofTop;
        bofLeaf->fVal       = 2;      
        fTree               = bofTop;
    }

    
    
    
    
    
    RBBINode *cn = new RBBINode(RBBINode::opCat);
    
    if (cn == NULL) {
        *fStatus = U_MEMORY_ALLOCATION_ERROR;
        return;
    }
    cn->fLeftChild = fTree;
    fTree->fParent = cn;
    cn->fRightChild = new RBBINode(RBBINode::endMark);
    
    if (cn->fRightChild == NULL) {
        *fStatus = U_MEMORY_ALLOCATION_ERROR;
        delete cn;
        return;
    }
    cn->fRightChild->fParent = cn;
    fTree = cn;

    
    
    
    
    fTree->flattenSets();
#ifdef RBBI_DEBUG
    if (fRB->fDebugEnv && uprv_strstr(fRB->fDebugEnv, "stree")) {
        RBBIDebugPuts("Parse tree after flattening Unicode Set references.");
        fTree->printTree(TRUE);
    }
#endif


    
    
    
    
    
    
    
    calcNullable(fTree);
    calcFirstPos(fTree);
    calcLastPos(fTree);
    calcFollowPos(fTree);
    if (fRB->fDebugEnv && uprv_strstr(fRB->fDebugEnv, "pos")) {
        RBBIDebugPuts("\n");
        printPosSets(fTree);
    }

    
    
    
    if (fRB->fChainRules) {
        calcChainedFollowPos(fTree);
    }

    
    
    
    if (fRB->fSetBuilder->sawBOF()) {
        bofFixup();
    }

    
    
    
    buildStateTable();
    flagAcceptingStates();
    flagLookAheadStates();
    flagTaggedStates();

    
    
    
    
    
    mergeRuleStatusVals();

    if (fRB->fDebugEnv && uprv_strstr(fRB->fDebugEnv, "states")) {printStates();};
}








void RBBITableBuilder::calcNullable(RBBINode *n) {
    if (n == NULL) {
        return;
    }
    if (n->fType == RBBINode::setRef ||
        n->fType == RBBINode::endMark ) {
        
        n->fNullable = FALSE;
        return;
    }

    if (n->fType == RBBINode::lookAhead || n->fType == RBBINode::tag) {
        
        
        n->fNullable = TRUE;
        return;
    }


    
    
    calcNullable(n->fLeftChild);
    calcNullable(n->fRightChild);

    
    if (n->fType == RBBINode::opOr) {
        n->fNullable = n->fLeftChild->fNullable || n->fRightChild->fNullable;
    }
    else if (n->fType == RBBINode::opCat) {
        n->fNullable = n->fLeftChild->fNullable && n->fRightChild->fNullable;
    }
    else if (n->fType == RBBINode::opStar || n->fType == RBBINode::opQuestion) {
        n->fNullable = TRUE;
    }
    else {
        n->fNullable = FALSE;
    }
}









void RBBITableBuilder::calcFirstPos(RBBINode *n) {
    if (n == NULL) {
        return;
    }
    if (n->fType == RBBINode::leafChar  ||
        n->fType == RBBINode::endMark   ||
        n->fType == RBBINode::lookAhead ||
        n->fType == RBBINode::tag) {
        
        
        
        
        n->fFirstPosSet->addElement(n, *fStatus);
        return;
    }

    
    
    calcFirstPos(n->fLeftChild);
    calcFirstPos(n->fRightChild);

    
    if (n->fType == RBBINode::opOr) {
        setAdd(n->fFirstPosSet, n->fLeftChild->fFirstPosSet);
        setAdd(n->fFirstPosSet, n->fRightChild->fFirstPosSet);
    }
    else if (n->fType == RBBINode::opCat) {
        setAdd(n->fFirstPosSet, n->fLeftChild->fFirstPosSet);
        if (n->fLeftChild->fNullable) {
            setAdd(n->fFirstPosSet, n->fRightChild->fFirstPosSet);
        }
    }
    else if (n->fType == RBBINode::opStar ||
             n->fType == RBBINode::opQuestion ||
             n->fType == RBBINode::opPlus) {
        setAdd(n->fFirstPosSet, n->fLeftChild->fFirstPosSet);
    }
}








void RBBITableBuilder::calcLastPos(RBBINode *n) {
    if (n == NULL) {
        return;
    }
    if (n->fType == RBBINode::leafChar  ||
        n->fType == RBBINode::endMark   ||
        n->fType == RBBINode::lookAhead ||
        n->fType == RBBINode::tag) {
        
        
        
        
        n->fLastPosSet->addElement(n, *fStatus);
        return;
    }

    
    
    calcLastPos(n->fLeftChild);
    calcLastPos(n->fRightChild);

    
    if (n->fType == RBBINode::opOr) {
        setAdd(n->fLastPosSet, n->fLeftChild->fLastPosSet);
        setAdd(n->fLastPosSet, n->fRightChild->fLastPosSet);
    }
    else if (n->fType == RBBINode::opCat) {
        setAdd(n->fLastPosSet, n->fRightChild->fLastPosSet);
        if (n->fRightChild->fNullable) {
            setAdd(n->fLastPosSet, n->fLeftChild->fLastPosSet);
        }
    }
    else if (n->fType == RBBINode::opStar     ||
             n->fType == RBBINode::opQuestion ||
             n->fType == RBBINode::opPlus) {
        setAdd(n->fLastPosSet, n->fLeftChild->fLastPosSet);
    }
}








void RBBITableBuilder::calcFollowPos(RBBINode *n) {
    if (n == NULL ||
        n->fType == RBBINode::leafChar ||
        n->fType == RBBINode::endMark) {
        return;
    }

    calcFollowPos(n->fLeftChild);
    calcFollowPos(n->fRightChild);

    
    if (n->fType == RBBINode::opCat) {
        RBBINode *i;   
        uint32_t     ix;

        UVector *LastPosOfLeftChild = n->fLeftChild->fLastPosSet;

        for (ix=0; ix<(uint32_t)LastPosOfLeftChild->size(); ix++) {
            i = (RBBINode *)LastPosOfLeftChild->elementAt(ix);
            setAdd(i->fFollowPos, n->fRightChild->fFirstPosSet);
        }
    }

    
    if (n->fType == RBBINode::opStar ||
        n->fType == RBBINode::opPlus) {
        RBBINode   *i;  
        uint32_t    ix;

        for (ix=0; ix<(uint32_t)n->fLastPosSet->size(); ix++) {
            i = (RBBINode *)n->fLastPosSet->elementAt(ix);
            setAdd(i->fFollowPos, n->fFirstPosSet);
        }
    }



}








void RBBITableBuilder::calcChainedFollowPos(RBBINode *tree) {

    UVector         endMarkerNodes(*fStatus);
    UVector         leafNodes(*fStatus);
    int32_t         i;

    if (U_FAILURE(*fStatus)) {
        return;
    }

    
    tree->findNodes(&endMarkerNodes, RBBINode::endMark, *fStatus);

    
    tree->findNodes(&leafNodes, RBBINode::leafChar, *fStatus);
    if (U_FAILURE(*fStatus)) {
        return;
    }

    
    
    
    RBBINode *userRuleRoot = tree;
    if (fRB->fSetBuilder->sawBOF()) {
        userRuleRoot = tree->fLeftChild->fRightChild;
    }
    U_ASSERT(userRuleRoot != NULL);
    UVector *matchStartNodes = userRuleRoot->fFirstPosSet;


    
    
    int32_t  endNodeIx;
    int32_t  startNodeIx;

    for (endNodeIx=0; endNodeIx<leafNodes.size(); endNodeIx++) {
        RBBINode *tNode   = (RBBINode *)leafNodes.elementAt(endNodeIx);
        RBBINode *endNode = NULL;

        
        
        for (i=0; i<endMarkerNodes.size(); i++) {
            if (tNode->fFollowPos->contains(endMarkerNodes.elementAt(i))) {
                endNode = tNode;
                break;
            }
        }
        if (endNode == NULL) {
            
            continue;
        }

        

        
        
        
        
        if (fRB->fLBCMNoChain) {
            UChar32 c = this->fRB->fSetBuilder->getFirstChar(endNode->fVal);
            if (c != -1) {
                
                ULineBreak cLBProp = (ULineBreak)u_getIntPropertyValue(c, UCHAR_LINE_BREAK);
                if (cLBProp == U_LB_COMBINING_MARK) {
                    continue;
                }
            }
        }


        
        
        RBBINode *startNode;
        for (startNodeIx = 0; startNodeIx<matchStartNodes->size(); startNodeIx++) {
            startNode = (RBBINode *)matchStartNodes->elementAt(startNodeIx);
            if (startNode->fType != RBBINode::leafChar) {
                continue;
            }

            if (endNode->fVal == startNode->fVal) {
                
                

                
                
                
                
                setAdd(endNode->fFollowPos, startNode->fFollowPos);
            }
        }
    }
}












void RBBITableBuilder::bofFixup() {

    if (U_FAILURE(*fStatus)) {
        return;
    }

    
    
    
    
    
    
    
    
    
    
    RBBINode  *bofNode = fTree->fLeftChild->fLeftChild;
    U_ASSERT(bofNode->fType == RBBINode::leafChar);
    U_ASSERT(bofNode->fVal == 2);

    
    
    
    
    
    UVector *matchStartNodes = fTree->fLeftChild->fRightChild->fFirstPosSet;

    RBBINode *startNode;
    int       startNodeIx;
    for (startNodeIx = 0; startNodeIx<matchStartNodes->size(); startNodeIx++) {
        startNode = (RBBINode *)matchStartNodes->elementAt(startNodeIx);
        if (startNode->fType != RBBINode::leafChar) {
            continue;
        }

        if (startNode->fVal == bofNode->fVal) {
            
            
            
            
            
            setAdd(bofNode->fFollowPos, startNode->fFollowPos);
        }
    }
}










void RBBITableBuilder::buildStateTable() {
    if (U_FAILURE(*fStatus)) {
        return;
    }
    RBBIStateDescriptor *failState;
    
    RBBIStateDescriptor *initialState = NULL; 
    
    
    int      lastInputSymbol = fRB->fSetBuilder->getNumCharCategories() - 1;
    failState = new RBBIStateDescriptor(lastInputSymbol, fStatus);
    if (failState == NULL) {
        *fStatus = U_MEMORY_ALLOCATION_ERROR;
        goto ExitBuildSTdeleteall;
    }
    failState->fPositions = new UVector(*fStatus);
    if (failState->fPositions == NULL) {
        *fStatus = U_MEMORY_ALLOCATION_ERROR;
    }
    if (failState->fPositions == NULL || U_FAILURE(*fStatus)) {
        goto ExitBuildSTdeleteall;
    }
    fDStates->addElement(failState, *fStatus);
    if (U_FAILURE(*fStatus)) {
        goto ExitBuildSTdeleteall;
    }

    
    
    initialState = new RBBIStateDescriptor(lastInputSymbol, fStatus);
    if (initialState == NULL) {
        *fStatus = U_MEMORY_ALLOCATION_ERROR;
    }
    if (U_FAILURE(*fStatus)) {
        goto ExitBuildSTdeleteall;
    }
    initialState->fPositions = new UVector(*fStatus);
    if (initialState->fPositions == NULL) {
        *fStatus = U_MEMORY_ALLOCATION_ERROR;
    }
    if (U_FAILURE(*fStatus)) {
        goto ExitBuildSTdeleteall;
    }
    setAdd(initialState->fPositions, fTree->fFirstPosSet);
    fDStates->addElement(initialState, *fStatus);
    if (U_FAILURE(*fStatus)) {
        goto ExitBuildSTdeleteall;
    }

    
    for (;;) {
        RBBIStateDescriptor *T = NULL;
        int32_t              tx;
        for (tx=1; tx<fDStates->size(); tx++) {
            RBBIStateDescriptor *temp;
            temp = (RBBIStateDescriptor *)fDStates->elementAt(tx);
            if (temp->fMarked == FALSE) {
                T = temp;
                break;
            }
        }
        if (T == NULL) {
            break;
        }

        
        T->fMarked = TRUE;

        
        int32_t  a;
        for (a = 1; a<=lastInputSymbol; a++) {
            
            
            
            UVector    *U = NULL;
            RBBINode   *p;
            int32_t     px;
            for (px=0; px<T->fPositions->size(); px++) {
                p = (RBBINode *)T->fPositions->elementAt(px);
                if ((p->fType == RBBINode::leafChar) &&  (p->fVal == a)) {
                    if (U == NULL) {
                        U = new UVector(*fStatus);
                        if (U == NULL) {
                        	*fStatus = U_MEMORY_ALLOCATION_ERROR;
                        	goto ExitBuildSTdeleteall;
                        }
                    }
                    setAdd(U, p->fFollowPos);
                }
            }

            
            int32_t  ux = 0;
            UBool    UinDstates = FALSE;
            if (U != NULL) {
                U_ASSERT(U->size() > 0);
                int  ix;
                for (ix=0; ix<fDStates->size(); ix++) {
                    RBBIStateDescriptor *temp2;
                    temp2 = (RBBIStateDescriptor *)fDStates->elementAt(ix);
                    if (setEquals(U, temp2->fPositions)) {
                        delete U;
                        U  = temp2->fPositions;
                        ux = ix;
                        UinDstates = TRUE;
                        break;
                    }
                }

                
                if (!UinDstates)
                {
                    RBBIStateDescriptor *newState = new RBBIStateDescriptor(lastInputSymbol, fStatus);
                    if (newState == NULL) {
                    	*fStatus = U_MEMORY_ALLOCATION_ERROR;
                    }
                    if (U_FAILURE(*fStatus)) {
                        goto ExitBuildSTdeleteall;
                    }
                    newState->fPositions = U;
                    fDStates->addElement(newState, *fStatus);
                    if (U_FAILURE(*fStatus)) {
                        return;
                    }
                    ux = fDStates->size()-1;
                }

                
                T->fDtran->setElementAt(ux, a);
            }
        }
    }
    return;
    
ExitBuildSTdeleteall:
    delete initialState;
    delete failState;
}












void     RBBITableBuilder::flagAcceptingStates() {
    if (U_FAILURE(*fStatus)) {
        return;
    }
    UVector     endMarkerNodes(*fStatus);
    RBBINode    *endMarker;
    int32_t     i;
    int32_t     n;

    if (U_FAILURE(*fStatus)) {
        return;
    }

    fTree->findNodes(&endMarkerNodes, RBBINode::endMark, *fStatus);
    if (U_FAILURE(*fStatus)) {
        return;
    }

    for (i=0; i<endMarkerNodes.size(); i++) {
        endMarker = (RBBINode *)endMarkerNodes.elementAt(i);
        for (n=0; n<fDStates->size(); n++) {
            RBBIStateDescriptor *sd = (RBBIStateDescriptor *)fDStates->elementAt(n);
            if (sd->fPositions->indexOf(endMarker) >= 0) {
                
                
                

                if (sd->fAccepting==0) {
                    
                    sd->fAccepting = endMarker->fVal;
                    if (sd->fAccepting == 0) {
                        sd->fAccepting = -1;
                    }
                }
                if (sd->fAccepting==-1 && endMarker->fVal != 0) {
                    
                    
                    
                    sd->fAccepting = endMarker->fVal;
                }
                
                

                
                
                if (endMarker->fLookAheadEnd) {
                    
                    
                    
                    sd->fLookAhead = sd->fAccepting;
                }
            }
        }
    }
}







void     RBBITableBuilder::flagLookAheadStates() {
    if (U_FAILURE(*fStatus)) {
        return;
    }
    UVector     lookAheadNodes(*fStatus);
    RBBINode    *lookAheadNode;
    int32_t     i;
    int32_t     n;

    fTree->findNodes(&lookAheadNodes, RBBINode::lookAhead, *fStatus);
    if (U_FAILURE(*fStatus)) {
        return;
    }
    for (i=0; i<lookAheadNodes.size(); i++) {
        lookAheadNode = (RBBINode *)lookAheadNodes.elementAt(i);

        for (n=0; n<fDStates->size(); n++) {
            RBBIStateDescriptor *sd = (RBBIStateDescriptor *)fDStates->elementAt(n);
            if (sd->fPositions->indexOf(lookAheadNode) >= 0) {
                sd->fLookAhead = lookAheadNode->fVal;
            }
        }
    }
}









void     RBBITableBuilder::flagTaggedStates() {
    if (U_FAILURE(*fStatus)) {
        return;
    }
    UVector     tagNodes(*fStatus);
    RBBINode    *tagNode;
    int32_t     i;
    int32_t     n;

    if (U_FAILURE(*fStatus)) {
        return;
    }
    fTree->findNodes(&tagNodes, RBBINode::tag, *fStatus);
    if (U_FAILURE(*fStatus)) {
        return;
    }
    for (i=0; i<tagNodes.size(); i++) {                   
        tagNode = (RBBINode *)tagNodes.elementAt(i);

        for (n=0; n<fDStates->size(); n++) {              
            RBBIStateDescriptor *sd = (RBBIStateDescriptor *)fDStates->elementAt(n);
            if (sd->fPositions->indexOf(tagNode) >= 0) {  
                sortedAdd(&sd->fTagVals, tagNode->fVal);
            }
        }
    }
}













void  RBBITableBuilder::mergeRuleStatusVals() {
    
    
    
    
    
    
    
    
    
    
    int i;
    int n;

    
    
    if (fRB->fRuleStatusVals->size() == 0) {
        fRB->fRuleStatusVals->addElement(1, *fStatus);  
        fRB->fRuleStatusVals->addElement((int32_t)0, *fStatus);  
    }

    
    for (n=0; n<fDStates->size(); n++) {
        RBBIStateDescriptor *sd = (RBBIStateDescriptor *)fDStates->elementAt(n);
        UVector *thisStatesTagValues = sd->fTagVals;
        if (thisStatesTagValues == NULL) {
            
            
            sd->fTagsIdx = 0;
            continue;
        }

        
        
        
        sd->fTagsIdx = -1;
        int32_t  thisTagGroupStart = 0;   
        int32_t  nextTagGroupStart = 0;

        
        while (nextTagGroupStart < fRB->fRuleStatusVals->size()) {
            thisTagGroupStart = nextTagGroupStart;
            nextTagGroupStart += fRB->fRuleStatusVals->elementAti(thisTagGroupStart) + 1;
            if (thisStatesTagValues->size() != fRB->fRuleStatusVals->elementAti(thisTagGroupStart)) {
                
                
                
                continue;
            }
            
            
            for (i=0; i<thisStatesTagValues->size(); i++) {
                if (thisStatesTagValues->elementAti(i) !=
                    fRB->fRuleStatusVals->elementAti(thisTagGroupStart + 1 + i) ) {
                    
                    break;
                }
            }

            if (i == thisStatesTagValues->size()) {
                
                
                sd->fTagsIdx = thisTagGroupStart;
                break;
            }
        }

        if (sd->fTagsIdx == -1) {
            
            sd->fTagsIdx = fRB->fRuleStatusVals->size();
            fRB->fRuleStatusVals->addElement(thisStatesTagValues->size(), *fStatus);
            for (i=0; i<thisStatesTagValues->size(); i++) {
                fRB->fRuleStatusVals->addElement(thisStatesTagValues->elementAti(i), *fStatus);
            }
        }
    }
}















void RBBITableBuilder::sortedAdd(UVector **vector, int32_t val) {
    int32_t i;

    if (*vector == NULL) {
        *vector = new UVector(*fStatus);
    }
    if (*vector == NULL || U_FAILURE(*fStatus)) {
        return;
    }
    UVector *vec = *vector;
    int32_t  vSize = vec->size();
    for (i=0; i<vSize; i++) {
        int32_t valAtI = vec->elementAti(i);
        if (valAtI == val) {
            
            return;
        }
        if (valAtI > val) {
            break;
        }
    }
    vec->insertElementAt(val, i, *fStatus);
}










void RBBITableBuilder::setAdd(UVector *dest, UVector *source) {
    int32_t destOriginalSize = dest->size();
    int32_t sourceSize       = source->size();
    int32_t di           = 0;
    MaybeStackArray<void *, 16> destArray, sourceArray;  
    void **destPtr, **sourcePtr;
    void **destLim, **sourceLim;

    if (destOriginalSize > destArray.getCapacity()) {
        if (destArray.resize(destOriginalSize) == NULL) {
            return;
        }
    }
    destPtr = destArray.getAlias();
    destLim = destPtr + destOriginalSize;  

    if (sourceSize > sourceArray.getCapacity()) {
        if (sourceArray.resize(sourceSize) == NULL) {
            return;
        }
    }
    sourcePtr = sourceArray.getAlias();
    sourceLim = sourcePtr + sourceSize;  

    
    (void) dest->toArray(destPtr);
    (void) source->toArray(sourcePtr);

    dest->setSize(sourceSize+destOriginalSize, *fStatus);

    while (sourcePtr < sourceLim && destPtr < destLim) {
        if (*destPtr == *sourcePtr) {
            dest->setElementAt(*sourcePtr++, di++);
            destPtr++;
        }
        
        
        else if (uprv_memcmp(destPtr, sourcePtr, sizeof(void *)) < 0) {
            dest->setElementAt(*destPtr++, di++);
        }
        else { 
            dest->setElementAt(*sourcePtr++, di++);
        }
    }

    
    while (destPtr < destLim) {
        dest->setElementAt(*destPtr++, di++);
    }
    while (sourcePtr < sourceLim) {
        dest->setElementAt(*sourcePtr++, di++);
    }

    dest->setSize(di, *fStatus);
}










UBool RBBITableBuilder::setEquals(UVector *a, UVector *b) {
    return a->equals(*b);
}








#ifdef RBBI_DEBUG
void RBBITableBuilder::printPosSets(RBBINode *n) {
    if (n==NULL) {
        return;
    }
    n->printNode();
    RBBIDebugPrintf("         Nullable:  %s\n", n->fNullable?"TRUE":"FALSE");

    RBBIDebugPrintf("         firstpos:  ");
    printSet(n->fFirstPosSet);

    RBBIDebugPrintf("         lastpos:   ");
    printSet(n->fLastPosSet);

    RBBIDebugPrintf("         followpos: ");
    printSet(n->fFollowPos);

    printPosSets(n->fLeftChild);
    printPosSets(n->fRightChild);
}
#endif









int32_t  RBBITableBuilder::getTableSize() const {
    int32_t    size = 0;
    int32_t    numRows;
    int32_t    numCols;
    int32_t    rowSize;

    if (fTree == NULL) {
        return 0;
    }

    size    = sizeof(RBBIStateTable) - 4;    

    numRows = fDStates->size();
    numCols = fRB->fSetBuilder->getNumCharCategories();

    
    
    
    rowSize = sizeof(RBBIStateTableRow) + sizeof(uint16_t)*(numCols-2);
    size   += numRows * rowSize;
    return size;
}










void RBBITableBuilder::exportTable(void *where) {
    RBBIStateTable    *table = (RBBIStateTable *)where;
    uint32_t           state;
    int                col;

    if (U_FAILURE(*fStatus) || fTree == NULL) {
        return;
    }

    if (fRB->fSetBuilder->getNumCharCategories() > 0x7fff ||
        fDStates->size() > 0x7fff) {
        *fStatus = U_BRK_INTERNAL_ERROR;
        return;
    }

    table->fRowLen    = sizeof(RBBIStateTableRow) +
                            sizeof(uint16_t) * (fRB->fSetBuilder->getNumCharCategories() - 2);
    table->fNumStates = fDStates->size();
    table->fFlags     = 0;
    if (fRB->fLookAheadHardBreak) {
        table->fFlags  |= RBBI_LOOKAHEAD_HARD_BREAK;
    }
    if (fRB->fSetBuilder->sawBOF()) {
        table->fFlags  |= RBBI_BOF_REQUIRED;
    }
    table->fReserved  = 0;

    for (state=0; state<table->fNumStates; state++) {
        RBBIStateDescriptor *sd = (RBBIStateDescriptor *)fDStates->elementAt(state);
        RBBIStateTableRow   *row = (RBBIStateTableRow *)(table->fTableData + state*table->fRowLen);
        U_ASSERT (-32768 < sd->fAccepting && sd->fAccepting <= 32767);
        U_ASSERT (-32768 < sd->fLookAhead && sd->fLookAhead <= 32767);
        row->fAccepting = (int16_t)sd->fAccepting;
        row->fLookAhead = (int16_t)sd->fLookAhead;
        row->fTagIdx    = (int16_t)sd->fTagsIdx;
        for (col=0; col<fRB->fSetBuilder->getNumCharCategories(); col++) {
            row->fNextState[col] = (uint16_t)sd->fDtran->elementAti(col);
        }
    }
}








#ifdef RBBI_DEBUG
void RBBITableBuilder::printSet(UVector *s) {
    int32_t  i;
    for (i=0; i<s->size(); i++) {
        void *v = s->elementAt(i);
        RBBIDebugPrintf("%10p", v);
    }
    RBBIDebugPrintf("\n");
}
#endif







#ifdef RBBI_DEBUG
void RBBITableBuilder::printStates() {
    int     c;    
    int     n;    

    RBBIDebugPrintf("state |           i n p u t     s y m b o l s \n");
    RBBIDebugPrintf("      | Acc  LA    Tag");
    for (c=0; c<fRB->fSetBuilder->getNumCharCategories(); c++) {
        RBBIDebugPrintf(" %2d", c);
    }
    RBBIDebugPrintf("\n");
    RBBIDebugPrintf("      |---------------");
    for (c=0; c<fRB->fSetBuilder->getNumCharCategories(); c++) {
        RBBIDebugPrintf("---");
    }
    RBBIDebugPrintf("\n");

    for (n=0; n<fDStates->size(); n++) {
        RBBIStateDescriptor *sd = (RBBIStateDescriptor *)fDStates->elementAt(n);
        RBBIDebugPrintf("  %3d | " , n);
        RBBIDebugPrintf("%3d %3d %5d ", sd->fAccepting, sd->fLookAhead, sd->fTagsIdx);
        for (c=0; c<fRB->fSetBuilder->getNumCharCategories(); c++) {
            RBBIDebugPrintf(" %2d", sd->fDtran->elementAti(c));
        }
        RBBIDebugPrintf("\n");
    }
    RBBIDebugPrintf("\n\n");
}
#endif








#ifdef RBBI_DEBUG
void RBBITableBuilder::printRuleStatusTable() {
    int32_t  thisRecord = 0;
    int32_t  nextRecord = 0;
    int      i;
    UVector  *tbl = fRB->fRuleStatusVals;

    RBBIDebugPrintf("index |  tags \n");
    RBBIDebugPrintf("-------------------\n");

    while (nextRecord < tbl->size()) {
        thisRecord = nextRecord;
        nextRecord = thisRecord + tbl->elementAti(thisRecord) + 1;
        RBBIDebugPrintf("%4d   ", thisRecord);
        for (i=thisRecord+1; i<nextRecord; i++) {
            RBBIDebugPrintf("  %5d", tbl->elementAti(i));
        }
        RBBIDebugPrintf("\n");
    }
    RBBIDebugPrintf("\n\n");
}
#endif









RBBIStateDescriptor::RBBIStateDescriptor(int lastInputSymbol, UErrorCode *fStatus) {
    fMarked    = FALSE;
    fAccepting = 0;
    fLookAhead = 0;
    fTagsIdx   = 0;
    fTagVals   = NULL;
    fPositions = NULL;
    fDtran     = NULL;

    fDtran     = new UVector(lastInputSymbol+1, *fStatus);
    if (U_FAILURE(*fStatus)) {
        return;
    }
    if (fDtran == NULL) {
        *fStatus = U_MEMORY_ALLOCATION_ERROR;
        return;
    }
    fDtran->setSize(lastInputSymbol+1, *fStatus);    
                                           
                                           
                                           
}


RBBIStateDescriptor::~RBBIStateDescriptor() {
    delete       fPositions;
    delete       fDtran;
    delete       fTagVals;
    fPositions = NULL;
    fDtran     = NULL;
    fTagVals   = NULL;
}

U_NAMESPACE_END

#endif 
