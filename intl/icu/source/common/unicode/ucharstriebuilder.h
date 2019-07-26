













#ifndef __UCHARSTRIEBUILDER_H__
#define __UCHARSTRIEBUILDER_H__

#include "unicode/utypes.h"
#include "unicode/stringtriebuilder.h"
#include "unicode/ucharstrie.h"
#include "unicode/unistr.h"






U_NAMESPACE_BEGIN

class UCharsTrieElement;







class U_COMMON_API UCharsTrieBuilder : public StringTrieBuilder {
public:
    




    UCharsTrieBuilder(UErrorCode &errorCode);

    



    virtual ~UCharsTrieBuilder();

    













    UCharsTrieBuilder &add(const UnicodeString &s, int32_t value, UErrorCode &errorCode);

    














    UCharsTrie *build(UStringTrieBuildOption buildOption, UErrorCode &errorCode);

    




















    UnicodeString &buildUnicodeString(UStringTrieBuildOption buildOption, UnicodeString &result,
                                      UErrorCode &errorCode);

    





    UCharsTrieBuilder &clear() {
        strings.remove();
        elementsLength=0;
        ucharsLength=0;
        return *this;
    }

private:
    UCharsTrieBuilder(const UCharsTrieBuilder &other);  
    UCharsTrieBuilder &operator=(const UCharsTrieBuilder &other);  

    void buildUChars(UStringTrieBuildOption buildOption, UErrorCode &errorCode);

    virtual int32_t getElementStringLength(int32_t i) const;
    virtual UChar getElementUnit(int32_t i, int32_t unitIndex) const;
    virtual int32_t getElementValue(int32_t i) const;

    virtual int32_t getLimitOfLinearMatch(int32_t first, int32_t last, int32_t unitIndex) const;

    virtual int32_t countElementUnits(int32_t start, int32_t limit, int32_t unitIndex) const;
    virtual int32_t skipElementsBySomeUnits(int32_t i, int32_t unitIndex, int32_t count) const;
    virtual int32_t indexOfElementWithNextUnit(int32_t i, int32_t unitIndex, UChar unit) const;

    virtual UBool matchNodesCanHaveValues() const { return TRUE; }

    virtual int32_t getMaxBranchLinearSubNodeLength() const { return UCharsTrie::kMaxBranchLinearSubNodeLength; }
    virtual int32_t getMinLinearMatch() const { return UCharsTrie::kMinLinearMatch; }
    virtual int32_t getMaxLinearMatchLength() const { return UCharsTrie::kMaxLinearMatchLength; }

#ifndef U_HIDE_INTERNAL_API
    class UCTLinearMatchNode : public LinearMatchNode {
    public:
        UCTLinearMatchNode(const UChar *units, int32_t len, Node *nextNode);
        virtual UBool operator==(const Node &other) const;
        virtual void write(StringTrieBuilder &builder);
    private:
        const UChar *s;
    };
#endif

    virtual Node *createLinearMatchNode(int32_t i, int32_t unitIndex, int32_t length,
                                        Node *nextNode) const;

    UBool ensureCapacity(int32_t length);
    virtual int32_t write(int32_t unit);
    int32_t write(const UChar *s, int32_t length);
    virtual int32_t writeElementUnits(int32_t i, int32_t unitIndex, int32_t length);
    virtual int32_t writeValueAndFinal(int32_t i, UBool isFinal);
    virtual int32_t writeValueAndType(UBool hasValue, int32_t value, int32_t node);
    virtual int32_t writeDeltaTo(int32_t jumpTarget);

    UnicodeString strings;
    UCharsTrieElement *elements;
    int32_t elementsCapacity;
    int32_t elementsLength;

    
    
    UChar *uchars;
    int32_t ucharsCapacity;
    int32_t ucharsLength;
};

U_NAMESPACE_END

#endif  
