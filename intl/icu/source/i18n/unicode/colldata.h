











 
#ifndef COLL_DATA_H
#define COLL_DATA_H

#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

#include "unicode/uobject.h"
#include "unicode/ucol.h"

U_NAMESPACE_BEGIN

#ifndef U_HIDE_INTERNAL_API




#define KEY_BUFFER_SIZE 64

 



#define CELIST_BUFFER_SIZE 4









 



#define STRING_LIST_BUFFER_SIZE 16









 





class U_I18N_API CEList : public UObject
{
public:
    












    CEList(UCollator *coll, const UnicodeString &string, UErrorCode &status);

    



    ~CEList();

    






    int32_t size() const;

    








    uint32_t get(int32_t index) const;

    










    UBool matchesAt(int32_t offset, const CEList *other) const; 

    








    uint32_t &operator[](int32_t index) const;

    



    virtual UClassID getDynamicClassID() const;
    



    static UClassID getStaticClassID();

private:
    void add(uint32_t ce, UErrorCode &status);

    uint32_t ceBuffer[CELIST_BUFFER_SIZE];
    uint32_t *ces;
    int32_t listMax;
    int32_t listSize;

#ifdef INSTRUMENT_CELIST
    static int32_t _active;
    static int32_t _histogram[10];
#endif
};








class U_I18N_API StringList : public UObject
{
public:
    










    StringList(UErrorCode &status);

    




    ~StringList();

    







    void add(const UnicodeString *string, UErrorCode &status);

    








    void add(const UChar *chars, int32_t count, UErrorCode &status);

    









    const UnicodeString *get(int32_t index) const;

    






    int32_t size() const;

    



    virtual UClassID getDynamicClassID() const;
    



    static UClassID getStaticClassID();

private:
    UnicodeString *strings;
    int32_t listMax;
    int32_t listSize;

#ifdef INSTRUMENT_STRING_LIST
    static int32_t _lists;
    static int32_t _strings;
    static int32_t _histogram[101];
#endif
};
#endif  




class StringToCEsMap;
class CEToStringsMap;
class CollDataCache;

#ifndef U_HIDE_INTERNAL_API


















class U_I18N_API CollData : public UObject
{
public:
    














    static CollData *open(UCollator *collator, UErrorCode &status);

    






    static void close(CollData *collData);

    





    UCollator *getCollator() const;

    











    const StringList *getStringList(int32_t ce) const;

    










    const CEList *getCEList(const UnicodeString *string) const;

    






    void freeCEList(const CEList *list);

    










    int32_t minLengthInChars(const CEList *ces, int32_t offset) const;

 
    


















   int32_t minLengthInChars(const CEList *ces, int32_t offset, int32_t *history) const;

   



    virtual UClassID getDynamicClassID() const;
   



    static UClassID getStaticClassID();

    











    static void freeCollDataCache();

    






    static void flushCollDataCache();

private:
    friend class CollDataCache;
    friend class CollDataCacheEntry;

    CollData(UCollator *collator, char *cacheKey, int32_t cachekeyLength, UErrorCode &status);
    ~CollData();

    CollData();

    static char *getCollatorKey(UCollator *collator, char *buffer, int32_t bufferLength);

    static CollDataCache *getCollDataCache();

    UCollator      *coll;
    StringToCEsMap *charsToCEList;
    CEToStringsMap *ceToCharsStartingWith;

    char keyBuffer[KEY_BUFFER_SIZE];
    char *key;

    static CollDataCache *collDataCache;

    uint32_t minHan;
    uint32_t maxHan;

    uint32_t jamoLimits[4];
};
#endif  

U_NAMESPACE_END

#endif 
#endif 
