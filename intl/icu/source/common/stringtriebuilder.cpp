













#include "utypeinfo.h"  
#include "unicode/utypes.h"
#include "unicode/stringtriebuilder.h"
#include "uassert.h"
#include "uhash.h"

U_CDECL_BEGIN

static int32_t U_CALLCONV
hashStringTrieNode(const UHashTok key) {
    return icu::StringTrieBuilder::hashNode(key.pointer);
}

static UBool U_CALLCONV
equalStringTrieNodes(const UHashTok key1, const UHashTok key2) {
    return icu::StringTrieBuilder::equalNodes(key1.pointer, key2.pointer);
}

U_CDECL_END

U_NAMESPACE_BEGIN

StringTrieBuilder::StringTrieBuilder() : nodes(NULL) {}

StringTrieBuilder::~StringTrieBuilder() {
    deleteCompactBuilder();
}

void
StringTrieBuilder::createCompactBuilder(int32_t sizeGuess, UErrorCode &errorCode) {
    if(U_FAILURE(errorCode)) {
        return;
    }
    nodes=uhash_openSize(hashStringTrieNode, equalStringTrieNodes, NULL,
                         sizeGuess, &errorCode);
    if(U_SUCCESS(errorCode)) {
        if(nodes==NULL) {
          errorCode=U_MEMORY_ALLOCATION_ERROR;
        } else {
          uhash_setKeyDeleter(nodes, uprv_deleteUObject);
        }
    }
}

void
StringTrieBuilder::deleteCompactBuilder() {
    uhash_close(nodes);
    nodes=NULL;
}

void
StringTrieBuilder::build(UStringTrieBuildOption buildOption, int32_t elementsLength,
                       UErrorCode &errorCode) {
    if(buildOption==USTRINGTRIE_BUILD_FAST) {
        writeNode(0, elementsLength, 0);
    } else  {
        createCompactBuilder(2*elementsLength, errorCode);
        Node *root=makeNode(0, elementsLength, 0, errorCode);
        if(U_SUCCESS(errorCode)) {
            root->markRightEdgesFirst(-1);
            root->write(*this);
        }
        deleteCompactBuilder();
    }
}




int32_t
StringTrieBuilder::writeNode(int32_t start, int32_t limit, int32_t unitIndex) {
    UBool hasValue=FALSE;
    int32_t value=0;
    int32_t type;
    if(unitIndex==getElementStringLength(start)) {
        
        value=getElementValue(start++);
        if(start==limit) {
            return writeValueAndFinal(value, TRUE);  
        }
        hasValue=TRUE;
    }
    
    int32_t minUnit=getElementUnit(start, unitIndex);
    int32_t maxUnit=getElementUnit(limit-1, unitIndex);
    if(minUnit==maxUnit) {
        
        int32_t lastUnitIndex=getLimitOfLinearMatch(start, limit-1, unitIndex);
        writeNode(start, limit, lastUnitIndex);
        
        int32_t length=lastUnitIndex-unitIndex;
        int32_t maxLinearMatchLength=getMaxLinearMatchLength();
        while(length>maxLinearMatchLength) {
            lastUnitIndex-=maxLinearMatchLength;
            length-=maxLinearMatchLength;
            writeElementUnits(start, lastUnitIndex, maxLinearMatchLength);
            write(getMinLinearMatch()+maxLinearMatchLength-1);
        }
        writeElementUnits(start, unitIndex, length);
        type=getMinLinearMatch()+length-1;
    } else {
        
        int32_t length=countElementUnits(start, limit, unitIndex);
        
        writeBranchSubNode(start, limit, unitIndex, length);
        if(--length<getMinLinearMatch()) {
            type=length;
        } else {
            write(length);
            type=0;
        }
    }
    return writeValueAndType(hasValue, value, type);
}



int32_t
StringTrieBuilder::writeBranchSubNode(int32_t start, int32_t limit, int32_t unitIndex, int32_t length) {
    UChar middleUnits[kMaxSplitBranchLevels];
    int32_t lessThan[kMaxSplitBranchLevels];
    int32_t ltLength=0;
    while(length>getMaxBranchLinearSubNodeLength()) {
        
        
        int32_t i=skipElementsBySomeUnits(start, unitIndex, length/2);
        
        middleUnits[ltLength]=getElementUnit(i, unitIndex);  
        lessThan[ltLength]=writeBranchSubNode(start, i, unitIndex, length/2);
        ++ltLength;
        
        start=i;
        length=length-length/2;
    }
    
    int32_t starts[kMaxBranchLinearSubNodeLength];
    UBool isFinal[kMaxBranchLinearSubNodeLength-1];
    int32_t unitNumber=0;
    do {
        int32_t i=starts[unitNumber]=start;
        UChar unit=getElementUnit(i++, unitIndex);
        i=indexOfElementWithNextUnit(i, unitIndex, unit);
        isFinal[unitNumber]= start==i-1 && unitIndex+1==getElementStringLength(start);
        start=i;
    } while(++unitNumber<length-1);
    
    starts[unitNumber]=start;

    
    
    
    
    int32_t jumpTargets[kMaxBranchLinearSubNodeLength-1];
    do {
        --unitNumber;
        if(!isFinal[unitNumber]) {
            jumpTargets[unitNumber]=writeNode(starts[unitNumber], starts[unitNumber+1], unitIndex+1);
        }
    } while(unitNumber>0);
    
    
    unitNumber=length-1;
    writeNode(start, limit, unitIndex+1);
    int32_t offset=write(getElementUnit(start, unitIndex));
    
    while(--unitNumber>=0) {
        start=starts[unitNumber];
        int32_t value;
        if(isFinal[unitNumber]) {
            
            value=getElementValue(start);
        } else {
            
            value=offset-jumpTargets[unitNumber];
        }
        writeValueAndFinal(value, isFinal[unitNumber]);
        offset=write(getElementUnit(start, unitIndex));
    }
    
    while(ltLength>0) {
        --ltLength;
        writeDeltaTo(lessThan[ltLength]);
        offset=write(middleUnits[ltLength]);
    }
    return offset;
}




StringTrieBuilder::Node *
StringTrieBuilder::makeNode(int32_t start, int32_t limit, int32_t unitIndex, UErrorCode &errorCode) {
    if(U_FAILURE(errorCode)) {
        return NULL;
    }
    UBool hasValue=FALSE;
    int32_t value=0;
    if(unitIndex==getElementStringLength(start)) {
        
        value=getElementValue(start++);
        if(start==limit) {
            return registerFinalValue(value, errorCode);
        }
        hasValue=TRUE;
    }
    Node *node;
    
    int32_t minUnit=getElementUnit(start, unitIndex);
    int32_t maxUnit=getElementUnit(limit-1, unitIndex);
    if(minUnit==maxUnit) {
        
        int32_t lastUnitIndex=getLimitOfLinearMatch(start, limit-1, unitIndex);
        Node *nextNode=makeNode(start, limit, lastUnitIndex, errorCode);
        
        int32_t length=lastUnitIndex-unitIndex;
        int32_t maxLinearMatchLength=getMaxLinearMatchLength();
        while(length>maxLinearMatchLength) {
            lastUnitIndex-=maxLinearMatchLength;
            length-=maxLinearMatchLength;
            node=createLinearMatchNode(start, lastUnitIndex, maxLinearMatchLength, nextNode);
            nextNode=registerNode(node, errorCode);
        }
        node=createLinearMatchNode(start, unitIndex, length, nextNode);
    } else {
        
        int32_t length=countElementUnits(start, limit, unitIndex);
        
        Node *subNode=makeBranchSubNode(start, limit, unitIndex, length, errorCode);
        node=new BranchHeadNode(length, subNode);
    }
    if(hasValue && node!=NULL) {
        if(matchNodesCanHaveValues()) {
            ((ValueNode *)node)->setValue(value);
        } else {
            node=new IntermediateValueNode(value, registerNode(node, errorCode));
        }
    }
    return registerNode(node, errorCode);
}



StringTrieBuilder::Node *
StringTrieBuilder::makeBranchSubNode(int32_t start, int32_t limit, int32_t unitIndex,
                                   int32_t length, UErrorCode &errorCode) {
    if(U_FAILURE(errorCode)) {
        return NULL;
    }
    UChar middleUnits[kMaxSplitBranchLevels];
    Node *lessThan[kMaxSplitBranchLevels];
    int32_t ltLength=0;
    while(length>getMaxBranchLinearSubNodeLength()) {
        
        
        int32_t i=skipElementsBySomeUnits(start, unitIndex, length/2);
        
        middleUnits[ltLength]=getElementUnit(i, unitIndex);  
        lessThan[ltLength]=makeBranchSubNode(start, i, unitIndex, length/2, errorCode);
        ++ltLength;
        
        start=i;
        length=length-length/2;
    }
    if(U_FAILURE(errorCode)) {
        return NULL;
    }
    ListBranchNode *listNode=new ListBranchNode();
    if(listNode==NULL) {
        errorCode=U_MEMORY_ALLOCATION_ERROR;
        return NULL;
    }
    
    int32_t unitNumber=0;
    do {
        int32_t i=start;
        UChar unit=getElementUnit(i++, unitIndex);
        i=indexOfElementWithNextUnit(i, unitIndex, unit);
        if(start==i-1 && unitIndex+1==getElementStringLength(start)) {
            listNode->add(unit, getElementValue(start));
        } else {
            listNode->add(unit, makeNode(start, i, unitIndex+1, errorCode));
        }
        start=i;
    } while(++unitNumber<length-1);
    
    UChar unit=getElementUnit(start, unitIndex);
    if(start==limit-1 && unitIndex+1==getElementStringLength(start)) {
        listNode->add(unit, getElementValue(start));
    } else {
        listNode->add(unit, makeNode(start, limit, unitIndex+1, errorCode));
    }
    Node *node=registerNode(listNode, errorCode);
    
    while(ltLength>0) {
        --ltLength;
        node=registerNode(
            new SplitBranchNode(middleUnits[ltLength], lessThan[ltLength], node), errorCode);
    }
    return node;
}

StringTrieBuilder::Node *
StringTrieBuilder::registerNode(Node *newNode, UErrorCode &errorCode) {
    if(U_FAILURE(errorCode)) {
        delete newNode;
        return NULL;
    }
    if(newNode==NULL) {
        errorCode=U_MEMORY_ALLOCATION_ERROR;
        return NULL;
    }
    const UHashElement *old=uhash_find(nodes, newNode);
    if(old!=NULL) {
        delete newNode;
        return (Node *)old->key.pointer;
    }
    
    
#if U_DEBUG
    int32_t oldValue=  
#endif
    uhash_puti(nodes, newNode, 1, &errorCode);
    U_ASSERT(oldValue==0);
    if(U_FAILURE(errorCode)) {
        delete newNode;
        return NULL;
    }
    return newNode;
}

StringTrieBuilder::Node *
StringTrieBuilder::registerFinalValue(int32_t value, UErrorCode &errorCode) {
    if(U_FAILURE(errorCode)) {
        return NULL;
    }
    FinalValueNode key(value);
    const UHashElement *old=uhash_find(nodes, &key);
    if(old!=NULL) {
        return (Node *)old->key.pointer;
    }
    Node *newNode=new FinalValueNode(value);
    if(newNode==NULL) {
        errorCode=U_MEMORY_ALLOCATION_ERROR;
        return NULL;
    }
    
    
#if U_DEBUG
    int32_t oldValue=  
#endif
    uhash_puti(nodes, newNode, 1, &errorCode);
    U_ASSERT(oldValue==0);
    if(U_FAILURE(errorCode)) {
        delete newNode;
        return NULL;
    }
    return newNode;
}

UBool
StringTrieBuilder::hashNode(const void *node) {
    return ((const Node *)node)->hashCode();
}

UBool
StringTrieBuilder::equalNodes(const void *left, const void *right) {
    return *(const Node *)left==*(const Node *)right;
}

UOBJECT_DEFINE_NO_RTTI_IMPLEMENTATION(StringTrieBuilder)

UBool
StringTrieBuilder::Node::operator==(const Node &other) const {
    return this==&other || (typeid(*this)==typeid(other) && hash==other.hash);
}

int32_t
StringTrieBuilder::Node::markRightEdgesFirst(int32_t edgeNumber) {
    if(offset==0) {
        offset=edgeNumber;
    }
    return edgeNumber;
}

UOBJECT_DEFINE_NO_RTTI_IMPLEMENTATION(StringTrieBuilder::Node)

UBool
StringTrieBuilder::FinalValueNode::operator==(const Node &other) const {
    if(this==&other) {
        return TRUE;
    }
    if(!Node::operator==(other)) {
        return FALSE;
    }
    const FinalValueNode &o=(const FinalValueNode &)other;
    return value==o.value;
}

void
StringTrieBuilder::FinalValueNode::write(StringTrieBuilder &builder) {
    offset=builder.writeValueAndFinal(value, TRUE);
}

UBool
StringTrieBuilder::ValueNode::operator==(const Node &other) const {
    if(this==&other) {
        return TRUE;
    }
    if(!Node::operator==(other)) {
        return FALSE;
    }
    const ValueNode &o=(const ValueNode &)other;
    return hasValue==o.hasValue && (!hasValue || value==o.value);
}

UBool
StringTrieBuilder::IntermediateValueNode::operator==(const Node &other) const {
    if(this==&other) {
        return TRUE;
    }
    if(!ValueNode::operator==(other)) {
        return FALSE;
    }
    const IntermediateValueNode &o=(const IntermediateValueNode &)other;
    return next==o.next;
}

int32_t
StringTrieBuilder::IntermediateValueNode::markRightEdgesFirst(int32_t edgeNumber) {
    if(offset==0) {
        offset=edgeNumber=next->markRightEdgesFirst(edgeNumber);
    }
    return edgeNumber;
}

void
StringTrieBuilder::IntermediateValueNode::write(StringTrieBuilder &builder) {
    next->write(builder);
    offset=builder.writeValueAndFinal(value, FALSE);
}

UBool
StringTrieBuilder::LinearMatchNode::operator==(const Node &other) const {
    if(this==&other) {
        return TRUE;
    }
    if(!ValueNode::operator==(other)) {
        return FALSE;
    }
    const LinearMatchNode &o=(const LinearMatchNode &)other;
    return length==o.length && next==o.next;
}

int32_t
StringTrieBuilder::LinearMatchNode::markRightEdgesFirst(int32_t edgeNumber) {
    if(offset==0) {
        offset=edgeNumber=next->markRightEdgesFirst(edgeNumber);
    }
    return edgeNumber;
}

UBool
StringTrieBuilder::ListBranchNode::operator==(const Node &other) const {
    if(this==&other) {
        return TRUE;
    }
    if(!Node::operator==(other)) {
        return FALSE;
    }
    const ListBranchNode &o=(const ListBranchNode &)other;
    for(int32_t i=0; i<length; ++i) {
        if(units[i]!=o.units[i] || values[i]!=o.values[i] || equal[i]!=o.equal[i]) {
            return FALSE;
        }
    }
    return TRUE;
}

int32_t
StringTrieBuilder::ListBranchNode::markRightEdgesFirst(int32_t edgeNumber) {
    if(offset==0) {
        firstEdgeNumber=edgeNumber;
        int32_t step=0;
        int32_t i=length;
        do {
            Node *edge=equal[--i];
            if(edge!=NULL) {
                edgeNumber=edge->markRightEdgesFirst(edgeNumber-step);
            }
            
            step=1;
        } while(i>0);
        offset=edgeNumber;
    }
    return edgeNumber;
}

void
StringTrieBuilder::ListBranchNode::write(StringTrieBuilder &builder) {
    
    
    
    
    int32_t unitNumber=length-1;
    Node *rightEdge=equal[unitNumber];
    int32_t rightEdgeNumber= rightEdge==NULL ? firstEdgeNumber : rightEdge->getOffset();
    do {
        --unitNumber;
        if(equal[unitNumber]!=NULL) {
            equal[unitNumber]->writeUnlessInsideRightEdge(firstEdgeNumber, rightEdgeNumber, builder);
        }
    } while(unitNumber>0);
    
    
    unitNumber=length-1;
    if(rightEdge==NULL) {
        builder.writeValueAndFinal(values[unitNumber], TRUE);
    } else {
        rightEdge->write(builder);
    }
    offset=builder.write(units[unitNumber]);
    
    while(--unitNumber>=0) {
        int32_t value;
        UBool isFinal;
        if(equal[unitNumber]==NULL) {
            
            value=values[unitNumber];
            isFinal=TRUE;
        } else {
            
            U_ASSERT(equal[unitNumber]->getOffset()>0);
            value=offset-equal[unitNumber]->getOffset();
            isFinal=FALSE;
        }
        builder.writeValueAndFinal(value, isFinal);
        offset=builder.write(units[unitNumber]);
    }
}

UBool
StringTrieBuilder::SplitBranchNode::operator==(const Node &other) const {
    if(this==&other) {
        return TRUE;
    }
    if(!Node::operator==(other)) {
        return FALSE;
    }
    const SplitBranchNode &o=(const SplitBranchNode &)other;
    return unit==o.unit && lessThan==o.lessThan && greaterOrEqual==o.greaterOrEqual;
}

int32_t
StringTrieBuilder::SplitBranchNode::markRightEdgesFirst(int32_t edgeNumber) {
    if(offset==0) {
        firstEdgeNumber=edgeNumber;
        edgeNumber=greaterOrEqual->markRightEdgesFirst(edgeNumber);
        offset=edgeNumber=lessThan->markRightEdgesFirst(edgeNumber-1);
    }
    return edgeNumber;
}

void
StringTrieBuilder::SplitBranchNode::write(StringTrieBuilder &builder) {
    
    lessThan->writeUnlessInsideRightEdge(firstEdgeNumber, greaterOrEqual->getOffset(), builder);
    
    greaterOrEqual->write(builder);
    
    U_ASSERT(lessThan->getOffset()>0);
    builder.writeDeltaTo(lessThan->getOffset());  
    offset=builder.write(unit);
}

UBool
StringTrieBuilder::BranchHeadNode::operator==(const Node &other) const {
    if(this==&other) {
        return TRUE;
    }
    if(!ValueNode::operator==(other)) {
        return FALSE;
    }
    const BranchHeadNode &o=(const BranchHeadNode &)other;
    return length==o.length && next==o.next;
}

int32_t
StringTrieBuilder::BranchHeadNode::markRightEdgesFirst(int32_t edgeNumber) {
    if(offset==0) {
        offset=edgeNumber=next->markRightEdgesFirst(edgeNumber);
    }
    return edgeNumber;
}

void
StringTrieBuilder::BranchHeadNode::write(StringTrieBuilder &builder) {
    next->write(builder);
    if(length<=builder.getMinLinearMatch()) {
        offset=builder.writeValueAndType(hasValue, value, length-1);
    } else {
        builder.write(length-1);
        offset=builder.writeValueAndType(hasValue, value, 0);
    }
}

U_NAMESPACE_END
