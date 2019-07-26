





#ifndef RBBINODE_H
#define RBBINODE_H

#include "unicode/utypes.h"
#include "unicode/uobject.h"








U_NAMESPACE_BEGIN

class    UnicodeSet;
class    UVector;

class RBBINode : public UMemory {
    public:
        enum NodeType {
            setRef,
            uset,
            varRef,
            leafChar,
            lookAhead,
            tag,
            endMark,
            opStart,
            opCat,
            opOr,
            opStar,
            opPlus,
            opQuestion,
            opBreak,
            opReverse,
            opLParen
        };

        enum OpPrecedence {      
            precZero,
            precStart,
            precLParen,
            precOpOr,
            precOpCat
        };
            
        NodeType      fType;
        RBBINode      *fParent;
        RBBINode      *fLeftChild;
        RBBINode      *fRightChild;
        UnicodeSet    *fInputSet;           
        OpPrecedence  fPrecedence;          
        
        UnicodeString fText;                
                                            
                                            
        int           fFirstPos;            
                                            
                                            
                                            
        int           fLastPos;             
                                            
                                            
                                            

        UBool         fNullable;            
        int32_t       fVal;                 
                                            
                                            
                                            

        UBool         fLookAheadEnd;        
                                            

        UVector       *fFirstPosSet;
        UVector       *fLastPosSet;         
        UVector       *fFollowPos;


        RBBINode(NodeType t);
        RBBINode(const RBBINode &other);
        ~RBBINode();
        
        RBBINode    *cloneTree();
        RBBINode    *flattenVariables();
        void         flattenSets();
        void         findNodes(UVector *dest, RBBINode::NodeType kind, UErrorCode &status);

#ifdef RBBI_DEBUG
        void        printNode();
        void        printTree(UBool withHeading);
#endif

    private:
        RBBINode &operator = (const RBBINode &other); 
        UBool operator == (const RBBINode &other);    

#ifdef RBBI_DEBUG
        int           fSerialNum;           
#endif
};

#ifdef RBBI_DEBUG
U_CFUNC void 
RBBI_DEBUG_printUnicodeString(const UnicodeString &s, int minWidth=0);
#endif

U_NAMESPACE_END

#endif

