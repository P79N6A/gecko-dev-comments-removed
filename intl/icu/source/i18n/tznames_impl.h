






#ifndef __TZNAMES_IMPL_H__
#define __TZNAMES_IMPL_H__







#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/tznames.h"
#include "unicode/ures.h"
#include "unicode/locid.h"
#include "uhash.h"
#include "uvector.h"
#include "umutex.h"

U_NAMESPACE_BEGIN





struct ZNStringPoolChunk;
class U_I18N_API ZNStringPool: public UMemory {
  public:
    ZNStringPool(UErrorCode &status);
    ~ZNStringPool();

    




    const UChar *get(const UChar *s, UErrorCode &status);

    


    const UChar *get(const UnicodeString &s, UErrorCode &status);

    


    const UChar *adopt(const UChar *s, UErrorCode &status);

    


    void freeze();

  private:
    ZNStringPoolChunk   *fChunks;
    UHashtable           *fHash;
};




struct CharacterNode {
    
    
    

    void clear();
    void deleteValues(UObjectDeleter *valueDeleter);

    void addValue(void *value, UObjectDeleter *valueDeleter, UErrorCode &status);
    inline UBool hasValues() const;
    inline int32_t countValues() const;
    inline const void *getValue(int32_t index) const;

    void     *fValues;      
    UChar    fCharacter;    
    uint16_t fFirstChild;   
    uint16_t fNextSibling;  
    UBool    fHasValuesVector;
    UBool    fPadding;

    
    
    
};

inline UBool CharacterNode::hasValues() const {
    return (UBool)(fValues != NULL);
}

inline int32_t CharacterNode::countValues() const {
    return
        fValues == NULL ? 0 :
        !fHasValuesVector ? 1 :
        ((const UVector *)fValues)->size();
}

inline const void *CharacterNode::getValue(int32_t index) const {
    if (!fHasValuesVector) {
        return fValues;  
    } else {
        return ((const UVector *)fValues)->elementAt(index);
    }
}




class TextTrieMapSearchResultHandler : public UMemory {
public:
    virtual UBool handleMatch(int32_t matchLength,
                              const CharacterNode *node, UErrorCode& status) = 0;
    virtual ~TextTrieMapSearchResultHandler(); 
};





class U_I18N_API TextTrieMap : public UMemory {
public:
    TextTrieMap(UBool ignoreCase, UObjectDeleter *valeDeleter);
    virtual ~TextTrieMap();

    void put(const UnicodeString &key, void *value, ZNStringPool &sp, UErrorCode &status);
    void put(const UChar*, void *value, UErrorCode &status);
    void search(const UnicodeString &text, int32_t start,
        TextTrieMapSearchResultHandler *handler, UErrorCode& status) const;
    int32_t isEmpty() const;

private:
    UBool           fIgnoreCase;
    CharacterNode   *fNodes;
    int32_t         fNodesCapacity;
    int32_t         fNodesCount;

    UVector         *fLazyContents;
    UBool           fIsEmpty;
    UObjectDeleter  *fValueDeleter;

    UBool growNodes();
    CharacterNode* addChildNode(CharacterNode *parent, UChar c, UErrorCode &status);
    CharacterNode* getChildNode(CharacterNode *parent, UChar c) const;

    void putImpl(const UnicodeString &key, void *value, UErrorCode &status);
    void buildTrie(UErrorCode &status);
    void search(CharacterNode *node, const UnicodeString &text, int32_t start,
        int32_t index, TextTrieMapSearchResultHandler *handler, UErrorCode &status) const;
};



class ZNames;
class TZNames;
class TextTrieMap;

class TimeZoneNamesImpl : public TimeZoneNames {
public:
    TimeZoneNamesImpl(const Locale& locale, UErrorCode& status);

    virtual ~TimeZoneNamesImpl();

    virtual UBool operator==(const TimeZoneNames& other) const;
    virtual TimeZoneNames* clone() const;

    StringEnumeration* getAvailableMetaZoneIDs(UErrorCode& status) const;
    StringEnumeration* getAvailableMetaZoneIDs(const UnicodeString& tzID, UErrorCode& status) const;

    UnicodeString& getMetaZoneID(const UnicodeString& tzID, UDate date, UnicodeString& mzID) const;
    UnicodeString& getReferenceZoneID(const UnicodeString& mzID, const char* region, UnicodeString& tzID) const;

    UnicodeString& getMetaZoneDisplayName(const UnicodeString& mzID, UTimeZoneNameType type, UnicodeString& name) const;
    UnicodeString& getTimeZoneDisplayName(const UnicodeString& tzID, UTimeZoneNameType type, UnicodeString& name) const;

    UnicodeString& getExemplarLocationName(const UnicodeString& tzID, UnicodeString& name) const;

    TimeZoneNames::MatchInfoCollection* find(const UnicodeString& text, int32_t start, uint32_t types, UErrorCode& status) const;

private:

    Locale fLocale;

    UResourceBundle* fZoneStrings;

    UHashtable* fTZNamesMap;
    UHashtable* fMZNamesMap;

    UBool fNamesTrieFullyLoaded;
    TextTrieMap fNamesTrie;

    void initialize(const Locale& locale, UErrorCode& status);
    void cleanup();

    void loadStrings(const UnicodeString& tzCanonicalID);

    ZNames* loadMetaZoneNames(const UnicodeString& mzId);
    TZNames* loadTimeZoneNames(const UnicodeString& mzId);
};

U_NAMESPACE_END

#endif 

#endif 


