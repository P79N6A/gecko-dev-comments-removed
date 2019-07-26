













#ifndef __STRINGTRIEBUILDER_H__
#define __STRINGTRIEBUILDER_H__

#include "unicode/utypes.h"
#include "unicode/uobject.h"







struct UHashtable;
typedef struct UHashtable UHashtable;





enum UStringTrieBuildOption {
    



    USTRINGTRIE_BUILD_FAST,
    









    USTRINGTRIE_BUILD_SMALL
};

U_NAMESPACE_BEGIN







class U_COMMON_API StringTrieBuilder : public UObject {
public:
#ifndef U_HIDE_INTERNAL_API
    
    static UBool hashNode(const void *node);
    
    static UBool equalNodes(const void *left, const void *right);
#endif  

protected:
    
    
    
    StringTrieBuilder();
    
    virtual ~StringTrieBuilder();

#ifndef U_HIDE_INTERNAL_API
    
    void createCompactBuilder(int32_t sizeGuess, UErrorCode &errorCode);
    
    void deleteCompactBuilder();

    
    void build(UStringTrieBuildOption buildOption, int32_t elementsLength, UErrorCode &errorCode);

    
    int32_t writeNode(int32_t start, int32_t limit, int32_t unitIndex);
    
    int32_t writeBranchSubNode(int32_t start, int32_t limit, int32_t unitIndex, int32_t length);
#endif  

    class Node;

#ifndef U_HIDE_INTERNAL_API
    
    Node *makeNode(int32_t start, int32_t limit, int32_t unitIndex, UErrorCode &errorCode);
    
    Node *makeBranchSubNode(int32_t start, int32_t limit, int32_t unitIndex,
                            int32_t length, UErrorCode &errorCode);
#endif  

    
    virtual int32_t getElementStringLength(int32_t i) const = 0;
    
    virtual UChar getElementUnit(int32_t i, int32_t unitIndex) const = 0;
    
    virtual int32_t getElementValue(int32_t i) const = 0;

    
    
    
    virtual int32_t getLimitOfLinearMatch(int32_t first, int32_t last, int32_t unitIndex) const = 0;

    
    
    virtual int32_t countElementUnits(int32_t start, int32_t limit, int32_t unitIndex) const = 0;
    
    virtual int32_t skipElementsBySomeUnits(int32_t i, int32_t unitIndex, int32_t count) const = 0;
    
    virtual int32_t indexOfElementWithNextUnit(int32_t i, int32_t unitIndex, UChar unit) const = 0;

    
    virtual UBool matchNodesCanHaveValues() const = 0;

    
    virtual int32_t getMaxBranchLinearSubNodeLength() const = 0;
    
    virtual int32_t getMinLinearMatch() const = 0;
    
    virtual int32_t getMaxLinearMatchLength() const = 0;

#ifndef U_HIDE_INTERNAL_API
    
    
    static const int32_t kMaxBranchLinearSubNodeLength=5;

    
    
    
    static const int32_t kMaxSplitBranchLevels=14;

    









    Node *registerNode(Node *newNode, UErrorCode &errorCode);
    









    Node *registerFinalValue(int32_t value, UErrorCode &errorCode);

    















    
    
    UHashtable *nodes;

    
    class Node : public UObject {
    public:
        Node(int32_t initialHash) : hash(initialHash), offset(0) {}
        inline int32_t hashCode() const { return hash; }
        
        static inline int32_t hashCode(const Node *node) { return node==NULL ? 0 : node->hashCode(); }
        
        virtual UBool operator==(const Node &other) const;
        inline UBool operator!=(const Node &other) const { return !operator==(other); }
        


























        virtual int32_t markRightEdgesFirst(int32_t edgeNumber);
        
        virtual void write(StringTrieBuilder &builder) = 0;
        
        inline void writeUnlessInsideRightEdge(int32_t firstRight, int32_t lastRight,
                                               StringTrieBuilder &builder) {
            
            
            
            
            
            if(offset<0 && (offset<lastRight || firstRight<offset)) {
                write(builder);
            }
        }
        inline int32_t getOffset() const { return offset; }
    protected:
        int32_t hash;
        int32_t offset;
    private:
        
        virtual UClassID getDynamicClassID() const;
    };

    
    
    
    
    
    
    
    class FinalValueNode : public Node {
    public:
        FinalValueNode(int32_t v) : Node(0x111111*37+v), value(v) {}
        virtual UBool operator==(const Node &other) const;
        virtual void write(StringTrieBuilder &builder);
    protected:
        int32_t value;
    };

    


    class ValueNode : public Node {
    public:
        ValueNode(int32_t initialHash) : Node(initialHash), hasValue(FALSE), value(0) {}
        virtual UBool operator==(const Node &other) const;
        void setValue(int32_t v) {
            hasValue=TRUE;
            value=v;
            hash=hash*37+v;
        }
    protected:
        UBool hasValue;
        int32_t value;
    };

    


    class IntermediateValueNode : public ValueNode {
    public:
        IntermediateValueNode(int32_t v, Node *nextNode)
                : ValueNode(0x222222*37+hashCode(nextNode)), next(nextNode) { setValue(v); }
        virtual UBool operator==(const Node &other) const;
        virtual int32_t markRightEdgesFirst(int32_t edgeNumber);
        virtual void write(StringTrieBuilder &builder);
    protected:
        Node *next;
    };

    


    class LinearMatchNode : public ValueNode {
    public:
        LinearMatchNode(int32_t len, Node *nextNode)
                : ValueNode((0x333333*37+len)*37+hashCode(nextNode)),
                  length(len), next(nextNode) {}
        virtual UBool operator==(const Node &other) const;
        virtual int32_t markRightEdgesFirst(int32_t edgeNumber);
    protected:
        int32_t length;
        Node *next;
    };

    


    class BranchNode : public Node {
    public:
        BranchNode(int32_t initialHash) : Node(initialHash) {}
    protected:
        int32_t firstEdgeNumber;
    };

    


    class ListBranchNode : public BranchNode {
    public:
        ListBranchNode() : BranchNode(0x444444), length(0) {}
        virtual UBool operator==(const Node &other) const;
        virtual int32_t markRightEdgesFirst(int32_t edgeNumber);
        virtual void write(StringTrieBuilder &builder);
        
        void add(int32_t c, int32_t value) {
            units[length]=(UChar)c;
            equal[length]=NULL;
            values[length]=value;
            ++length;
            hash=(hash*37+c)*37+value;
        }
        
        void add(int32_t c, Node *node) {
            units[length]=(UChar)c;
            equal[length]=node;
            values[length]=0;
            ++length;
            hash=(hash*37+c)*37+hashCode(node);
        }
    protected:
        Node *equal[kMaxBranchLinearSubNodeLength];  
        int32_t length;
        int32_t values[kMaxBranchLinearSubNodeLength];
        UChar units[kMaxBranchLinearSubNodeLength];
    };

    


    class SplitBranchNode : public BranchNode {
    public:
        SplitBranchNode(UChar middleUnit, Node *lessThanNode, Node *greaterOrEqualNode)
                : BranchNode(((0x555555*37+middleUnit)*37+
                              hashCode(lessThanNode))*37+hashCode(greaterOrEqualNode)),
                  unit(middleUnit), lessThan(lessThanNode), greaterOrEqual(greaterOrEqualNode) {}
        virtual UBool operator==(const Node &other) const;
        virtual int32_t markRightEdgesFirst(int32_t edgeNumber);
        virtual void write(StringTrieBuilder &builder);
    protected:
        UChar unit;
        Node *lessThan;
        Node *greaterOrEqual;
    };

    
    
    class BranchHeadNode : public ValueNode {
    public:
        BranchHeadNode(int32_t len, Node *subNode)
                : ValueNode((0x666666*37+len)*37+hashCode(subNode)),
                  length(len), next(subNode) {}
        virtual UBool operator==(const Node &other) const;
        virtual int32_t markRightEdgesFirst(int32_t edgeNumber);
        virtual void write(StringTrieBuilder &builder);
    protected:
        int32_t length;
        Node *next;  
    };
#endif  

    
    virtual Node *createLinearMatchNode(int32_t i, int32_t unitIndex, int32_t length,
                                        Node *nextNode) const = 0;

    
    virtual int32_t write(int32_t unit) = 0;
    
    virtual int32_t writeElementUnits(int32_t i, int32_t unitIndex, int32_t length) = 0;
    
    virtual int32_t writeValueAndFinal(int32_t i, UBool isFinal) = 0;
    
    virtual int32_t writeValueAndType(UBool hasValue, int32_t value, int32_t node) = 0;
    
    virtual int32_t writeDeltaTo(int32_t jumpTarget) = 0;

private:
    
    virtual UClassID getDynamicClassID() const;
};

U_NAMESPACE_END

#endif  
