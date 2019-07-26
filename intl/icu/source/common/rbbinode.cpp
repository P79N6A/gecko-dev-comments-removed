
















#include "unicode/utypes.h"

#if !UCONFIG_NO_BREAK_ITERATION

#include "unicode/unistr.h"
#include "unicode/uniset.h"
#include "unicode/uchar.h"
#include "unicode/parsepos.h"
#include "uvector.h"

#include "rbbirb.h"
#include "rbbinode.h"

#include "uassert.h"


U_NAMESPACE_BEGIN

#ifdef RBBI_DEBUG
static int  gLastSerial = 0;
#endif







RBBINode::RBBINode(NodeType t) : UMemory() {
#ifdef RBBI_DEBUG
    fSerialNum    = ++gLastSerial;
#endif
    fType         = t;
    fParent       = NULL;
    fLeftChild    = NULL;
    fRightChild   = NULL;
    fInputSet     = NULL;
    fFirstPos     = 0;
    fLastPos      = 0;
    fNullable     = FALSE;
    fLookAheadEnd = FALSE;
    fVal          = 0;
    fPrecedence   = precZero;

    UErrorCode     status = U_ZERO_ERROR;
    fFirstPosSet  = new UVector(status);  
    fLastPosSet   = new UVector(status);
    fFollowPos    = new UVector(status);
    if      (t==opCat)    {fPrecedence = precOpCat;}
    else if (t==opOr)     {fPrecedence = precOpOr;}
    else if (t==opStart)  {fPrecedence = precStart;}
    else if (t==opLParen) {fPrecedence = precLParen;}

}


RBBINode::RBBINode(const RBBINode &other) : UMemory(other) {
#ifdef RBBI_DEBUG
    fSerialNum   = ++gLastSerial;
#endif
    fType        = other.fType;
    fParent      = NULL;
    fLeftChild   = NULL;
    fRightChild  = NULL;
    fInputSet    = other.fInputSet;
    fPrecedence  = other.fPrecedence;
    fText        = other.fText;
    fFirstPos    = other.fFirstPos;
    fLastPos     = other.fLastPos;
    fNullable    = other.fNullable;
    fVal         = other.fVal;
    UErrorCode     status = U_ZERO_ERROR;
    fFirstPosSet = new UVector(status);   
    fLastPosSet  = new UVector(status);
    fFollowPos   = new UVector(status);
}











RBBINode::~RBBINode() {
    
    delete fInputSet;
    fInputSet = NULL;

    switch (this->fType) {
    case varRef:
    case setRef:
        
        
        break;

    default:
        delete        fLeftChild;
        fLeftChild =   NULL;
        delete        fRightChild;
        fRightChild = NULL;
    }


    delete fFirstPosSet;
    delete fLastPosSet;
    delete fFollowPos;

}











RBBINode *RBBINode::cloneTree() {
    RBBINode    *n;

    if (fType == RBBINode::varRef) {
        
        
        n = fLeftChild->cloneTree();
    } else if (fType == RBBINode::uset) {
        n = this;
    } else {
        n = new RBBINode(*this);
        
        if (n != NULL) {
            if (fLeftChild != NULL) {
                n->fLeftChild          = fLeftChild->cloneTree();
                n->fLeftChild->fParent = n;
            }
            if (fRightChild != NULL) {
                n->fRightChild          = fRightChild->cloneTree();
                n->fRightChild->fParent = n;
            }
        }
    }
    return n;
}





















RBBINode *RBBINode::flattenVariables() {
    if (fType == varRef) {
        RBBINode *retNode = fLeftChild->cloneTree();
        delete this;
        return retNode;
    }

    if (fLeftChild != NULL) {
        fLeftChild = fLeftChild->flattenVariables();
        fLeftChild->fParent  = this;
    }
    if (fRightChild != NULL) {
        fRightChild = fRightChild->flattenVariables();
        fRightChild->fParent = this;
    }
    return this;
}










void RBBINode::flattenSets() {
    U_ASSERT(fType != setRef);

    if (fLeftChild != NULL) {
        if (fLeftChild->fType==setRef) {
            RBBINode *setRefNode = fLeftChild;
            RBBINode *usetNode   = setRefNode->fLeftChild;
            RBBINode *replTree   = usetNode->fLeftChild;
            fLeftChild           = replTree->cloneTree();
            fLeftChild->fParent  = this;
            delete setRefNode;
        } else {
            fLeftChild->flattenSets();
        }
    }

    if (fRightChild != NULL) {
        if (fRightChild->fType==setRef) {
            RBBINode *setRefNode = fRightChild;
            RBBINode *usetNode   = setRefNode->fLeftChild;
            RBBINode *replTree   = usetNode->fLeftChild;
            fRightChild           = replTree->cloneTree();
            fRightChild->fParent  = this;
            delete setRefNode;
        } else {
            fRightChild->flattenSets();
        }
    }
}









void   RBBINode::findNodes(UVector *dest, RBBINode::NodeType kind, UErrorCode &status) {
    
    if (U_FAILURE(status)) {
        return;
    }
    if (fType == kind) {
        dest->addElement(this, status);
    }
    if (fLeftChild != NULL) {
        fLeftChild->findNodes(dest, kind, status);
    }
    if (fRightChild != NULL) {
        fRightChild->findNodes(dest, kind, status);
    }
}







#ifdef RBBI_DEBUG
void RBBINode::printNode() {
    static const char * const nodeTypeNames[] = {
                "setRef",
                "uset",
                "varRef",
                "leafChar",
                "lookAhead",
                "tag",
                "endMark",
                "opStart",
                "opCat",
                "opOr",
                "opStar",
                "opPlus",
                "opQuestion",
                "opBreak",
                "opReverse",
                "opLParen"
    };

    if (this==NULL) {
        RBBIDebugPrintf("%10p", (void *)this);
    } else {
        RBBIDebugPrintf("%10p  %12s  %10p  %10p  %10p      %4d     %6d   %d ",
            (void *)this, nodeTypeNames[fType], (void *)fParent, (void *)fLeftChild, (void *)fRightChild,
            fSerialNum, fFirstPos, fVal);
        if (fType == varRef) {
            RBBI_DEBUG_printUnicodeString(fText);
        }
    }
    RBBIDebugPrintf("\n");
}
#endif


#ifdef RBBI_DEBUG
U_CFUNC void RBBI_DEBUG_printUnicodeString(const UnicodeString &s, int minWidth)
{
    int i;
    for (i=0; i<s.length(); i++) {
        RBBIDebugPrintf("%c", s.charAt(i));
        
    }
    for (i=s.length(); i<minWidth; i++) {
        RBBIDebugPrintf(" ");
    }
}
#endif







#ifdef RBBI_DEBUG
void RBBINode::printTree(UBool printHeading) {
    if (printHeading) {
        RBBIDebugPrintf( "-------------------------------------------------------------------\n"
                         "    Address       type         Parent   LeftChild  RightChild    serial  position value\n"
              );
    }
    this->printNode();
    if (this != NULL) {
        
        
        if (fType != varRef) {
            if (fLeftChild != NULL) {
                fLeftChild->printTree(FALSE);
            }
            
            if (fRightChild != NULL) {
                fRightChild->printTree(FALSE);
            }
        }
    }
}
#endif



U_NAMESPACE_END

#endif 
