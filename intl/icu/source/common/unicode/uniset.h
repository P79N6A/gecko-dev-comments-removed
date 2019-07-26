









#ifndef UNICODESET_H
#define UNICODESET_H

#include "unicode/unifilt.h"
#include "unicode/unistr.h"
#include "unicode/uset.h"






U_NAMESPACE_BEGIN

class BMPSet;
class ParsePosition;
class RBBIRuleScanner;
class SymbolTable;
class UnicodeSetStringSpan;
class UVector;
class RuleCharacterIterator;

















































































































































































































































class U_COMMON_API UnicodeSet : public UnicodeFilter {

    int32_t len; 
    int32_t capacity; 
    UChar32* list; 
    BMPSet *bmpSet; 
    UChar32* buffer; 
    int32_t bufferCapacity; 
    int32_t patLen;

    








    UChar *pat;
    UVector* strings; 
    UnicodeSetStringSpan *stringSpan;

private:
    enum { 
        kIsBogus = 1       
    };
    uint8_t fFlags;         
public:
    








    inline UBool isBogus(void) const;
    
    















    void setToBogus();

public:

    enum {
        



        MIN_VALUE = 0,

        



        MAX_VALUE = 0x10ffff
    };

    
    
    

public:

    



    UnicodeSet();

    







    UnicodeSet(UChar32 start, UChar32 end);

    







    UnicodeSet(const UnicodeString& pattern,
               UErrorCode& status);

#ifndef U_HIDE_INTERNAL_API
    











    UnicodeSet(const UnicodeString& pattern,
               uint32_t options,
               const SymbolTable* symbols,
               UErrorCode& status);
#endif  

    












    UnicodeSet(const UnicodeString& pattern, ParsePosition& pos,
               uint32_t options,
               const SymbolTable* symbols,
               UErrorCode& status);

    



    UnicodeSet(const UnicodeSet& o);

    



    virtual ~UnicodeSet();

    




    UnicodeSet& operator=(const UnicodeSet& o);

    










    virtual UBool operator==(const UnicodeSet& o) const;

    




    UBool operator!=(const UnicodeSet& o) const;

    








    virtual UnicodeFunctor* clone() const;

    






    virtual int32_t hashCode(void) const;

    







    inline static UnicodeSet *fromUSet(USet *uset);

    







    inline static const UnicodeSet *fromUSet(const USet *uset);
    
    






    inline USet *toUSet();


    






    inline const USet * toUSet() const;


    
    
    

    







    inline UBool isFrozen() const;

    












    UnicodeFunctor *freeze();

    







    UnicodeFunctor *cloneAsThawed() const;

    
    
    

    









    UnicodeSet& set(UChar32 start, UChar32 end);

    




    static UBool resemblesPattern(const UnicodeString& pattern,
                                  int32_t pos);

    











    UnicodeSet& applyPattern(const UnicodeString& pattern,
                             UErrorCode& status);

#ifndef U_HIDE_INTERNAL_API
    















    UnicodeSet& applyPattern(const UnicodeString& pattern,
                             uint32_t options,
                             const SymbolTable* symbols,
                             UErrorCode& status);
#endif  

    






























    UnicodeSet& applyPattern(const UnicodeString& pattern,
                             ParsePosition& pos,
                             uint32_t options,
                             const SymbolTable* symbols,
                             UErrorCode& status);

    












    virtual UnicodeString& toPattern(UnicodeString& result,
                             UBool escapeUnprintable = FALSE) const;

    





















    UnicodeSet& applyIntPropertyValue(UProperty prop,
                                      int32_t value,
                                      UErrorCode& ec);

    




























    UnicodeSet& applyPropertyAlias(const UnicodeString& prop,
                                   const UnicodeString& value,
                                   UErrorCode& ec);

    







    virtual int32_t size(void) const;

    





    virtual UBool isEmpty(void) const;

    






    virtual UBool contains(UChar32 c) const;

    







    virtual UBool contains(UChar32 start, UChar32 end) const;

    






    UBool contains(const UnicodeString& s) const;

    






    virtual UBool containsAll(const UnicodeSet& c) const;

    






    UBool containsAll(const UnicodeString& s) const;

    







    UBool containsNone(UChar32 start, UChar32 end) const;

    






    UBool containsNone(const UnicodeSet& c) const;

    






    UBool containsNone(const UnicodeString& s) const;

    







    inline UBool containsSome(UChar32 start, UChar32 end) const;

    






    inline UBool containsSome(const UnicodeSet& s) const;

    






    inline UBool containsSome(const UnicodeString& s) const;

    

















    int32_t span(const UChar *s, int32_t length, USetSpanCondition spanCondition) const;

    











    inline int32_t span(const UnicodeString &s, int32_t start, USetSpanCondition spanCondition) const;

    
















    int32_t spanBack(const UChar *s, int32_t length, USetSpanCondition spanCondition) const;

    












    inline int32_t spanBack(const UnicodeString &s, int32_t limit, USetSpanCondition spanCondition) const;

    

















    int32_t spanUTF8(const char *s, int32_t length, USetSpanCondition spanCondition) const;

    
















    int32_t spanBackUTF8(const char *s, int32_t length, USetSpanCondition spanCondition) const;

    



    virtual UMatchDegree matches(const Replaceable& text,
                         int32_t& offset,
                         int32_t limit,
                         UBool incremental);

private:
    





















    static int32_t matchRest(const Replaceable& text,
                             int32_t start, int32_t limit,
                             const UnicodeString& s);

    








    int32_t findCodePoint(UChar32 c) const;

public:

    






    virtual void addMatchSetTo(UnicodeSet& toUnionTo) const;

    







    int32_t indexOf(UChar32 c) const;

    








    UChar32 charAt(int32_t index) const;

    













    virtual UnicodeSet& add(UChar32 start, UChar32 end);

    






    UnicodeSet& add(UChar32 c);

    










    UnicodeSet& add(const UnicodeString& s);

 private:
    




    static int32_t getSingleCP(const UnicodeString& s);

    void _add(const UnicodeString& s);

 public:
    







    UnicodeSet& addAll(const UnicodeString& s);

    







    UnicodeSet& retainAll(const UnicodeString& s);

    







    UnicodeSet& complementAll(const UnicodeString& s);

    







    UnicodeSet& removeAll(const UnicodeString& s);

    







    static UnicodeSet* U_EXPORT2 createFrom(const UnicodeString& s);


    






    static UnicodeSet* U_EXPORT2 createFromAll(const UnicodeString& s);

    












    virtual UnicodeSet& retain(UChar32 start, UChar32 end);


    




    UnicodeSet& retain(UChar32 c);

    












    virtual UnicodeSet& remove(UChar32 start, UChar32 end);

    






    UnicodeSet& remove(UChar32 c);

    








    UnicodeSet& remove(const UnicodeString& s);

    






    virtual UnicodeSet& complement(void);

    













    virtual UnicodeSet& complement(UChar32 start, UChar32 end);

    






    UnicodeSet& complement(UChar32 c);

    









    UnicodeSet& complement(const UnicodeString& s);

    











    virtual UnicodeSet& addAll(const UnicodeSet& c);

    










    virtual UnicodeSet& retainAll(const UnicodeSet& c);

    










    virtual UnicodeSet& removeAll(const UnicodeSet& c);

    









    virtual UnicodeSet& complementAll(const UnicodeSet& c);

    





    virtual UnicodeSet& clear(void);

    
























    UnicodeSet& closeOver(int32_t attribute);

    





    virtual UnicodeSet &removeAllStrings();

    






    virtual int32_t getRangeCount(void) const;

    






    virtual UChar32 getRangeStart(int32_t index) const;

    






    virtual UChar32 getRangeEnd(int32_t index) const;

    















































    int32_t serialize(uint16_t *dest, int32_t destCapacity, UErrorCode& ec) const;

    





    virtual UnicodeSet& compact();

    










    static UClassID U_EXPORT2 getStaticClassID(void);

    







    virtual UClassID getDynamicClassID(void) const;

private:

    

    friend class USetAccess;

    int32_t getStringCount() const;

    const UnicodeString* getString(int32_t index) const;

    
    
    

private:

    




    virtual UBool matchesIndexValue(uint8_t v) const;

private:
    friend class RBBIRuleScanner;

    
    
    

    UnicodeSet(const UnicodeSet& o, UBool );

    
    
    

    void applyPatternIgnoreSpace(const UnicodeString& pattern,
                                 ParsePosition& pos,
                                 const SymbolTable* symbols,
                                 UErrorCode& status);

    void applyPattern(RuleCharacterIterator& chars,
                      const SymbolTable* symbols,
                      UnicodeString& rebuiltPat,
                      uint32_t options,
                      UnicodeSet& (UnicodeSet::*caseClosure)(int32_t attribute),
                      UErrorCode& ec);

    
    
    

    void ensureCapacity(int32_t newLen, UErrorCode& ec);

    void ensureBufferCapacity(int32_t newLen, UErrorCode& ec);

    void swapBuffers(void);

    UBool allocateStrings(UErrorCode &status);

    UnicodeString& _toPattern(UnicodeString& result,
                              UBool escapeUnprintable) const;

    UnicodeString& _generatePattern(UnicodeString& result,
                                    UBool escapeUnprintable) const;

    static void _appendToPat(UnicodeString& buf, const UnicodeString& s, UBool escapeUnprintable);

    static void _appendToPat(UnicodeString& buf, UChar32 c, UBool escapeUnprintable);

    
    
    

    void exclusiveOr(const UChar32* other, int32_t otherLen, int8_t polarity);

    void add(const UChar32* other, int32_t otherLen, int8_t polarity);

    void retain(const UChar32* other, int32_t otherLen, int8_t polarity);

    




    static UBool resemblesPropertyPattern(const UnicodeString& pattern,
                                          int32_t pos);

    static UBool resemblesPropertyPattern(RuleCharacterIterator& chars,
                                          int32_t iterOpts);

    






































    UnicodeSet& applyPropertyPattern(const UnicodeString& pattern,
                                     ParsePosition& ppos,
                                     UErrorCode &ec);

    void applyPropertyPattern(RuleCharacterIterator& chars,
                              UnicodeString& rebuiltPat,
                              UErrorCode& ec);

    static const UnicodeSet* getInclusions(int32_t src, UErrorCode &status);

    



    typedef UBool (*Filter)(UChar32 codePoint, void* context);

    








    void applyFilter(Filter filter,
                     void* context,
                     int32_t src,
                     UErrorCode &status);

    


    void setPattern(const UnicodeString& newPat);
    


    void releasePattern();

    friend class UnicodeSetIterator;
};



inline UBool UnicodeSet::operator!=(const UnicodeSet& o) const {
    return !operator==(o);
}

inline UBool UnicodeSet::isFrozen() const {
    return (UBool)(bmpSet!=NULL || stringSpan!=NULL);
}

inline UBool UnicodeSet::containsSome(UChar32 start, UChar32 end) const {
    return !containsNone(start, end);
}

inline UBool UnicodeSet::containsSome(const UnicodeSet& s) const {
    return !containsNone(s);
}

inline UBool UnicodeSet::containsSome(const UnicodeString& s) const {
    return !containsNone(s);
}

inline UBool UnicodeSet::isBogus() const {
    return (UBool)(fFlags & kIsBogus);
}

inline UnicodeSet *UnicodeSet::fromUSet(USet *uset) {
    return reinterpret_cast<UnicodeSet *>(uset);
}

inline const UnicodeSet *UnicodeSet::fromUSet(const USet *uset) {
    return reinterpret_cast<const UnicodeSet *>(uset);
}

inline USet *UnicodeSet::toUSet() {
    return reinterpret_cast<USet *>(this);
}

inline const USet *UnicodeSet::toUSet() const {
    return reinterpret_cast<const USet *>(this);
}

inline int32_t UnicodeSet::span(const UnicodeString &s, int32_t start, USetSpanCondition spanCondition) const {
    int32_t sLength=s.length();
    if(start<0) {
        start=0;
    } else if(start>sLength) {
        start=sLength;
    }
    return start+span(s.getBuffer()+start, sLength-start, spanCondition);
}

inline int32_t UnicodeSet::spanBack(const UnicodeString &s, int32_t limit, USetSpanCondition spanCondition) const {
    int32_t sLength=s.length();
    if(limit<0) {
        limit=0;
    } else if(limit>sLength) {
        limit=sLength;
    }
    return spanBack(s.getBuffer(), limit, spanCondition);
}

U_NAMESPACE_END

#endif
