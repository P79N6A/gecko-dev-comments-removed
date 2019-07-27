



















#ifndef COLL_DATA_H
#define COLL_DATA_H

#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

#include "unicode/ucol.h"
#include "unicode/unistr.h"

 


#define CELIST_BUFFER_SIZE 4







 


#define STRING_LIST_BUFFER_SIZE 16

U_NAMESPACE_USE

 




class CEList
{
public:
    










    CEList(UCollator *coll, const UnicodeString &string, UErrorCode &status);

    


    ~CEList();

    




    int32_t size() const;

    






    uint32_t get(int32_t index) const;

    








    UBool matchesAt(int32_t offset, const CEList *other) const; 

    






    uint32_t &operator[](int32_t index) const;

private:
    void add(uint32_t ce, UErrorCode &status);

    uint32_t ceBuffer[CELIST_BUFFER_SIZE];
    uint32_t *ces;
    int32_t listMax;
    int32_t listSize;
};






class StringList
{
public:
    








    StringList(UErrorCode &status);

    


    ~StringList();

    





    void add(const UnicodeString *string, UErrorCode &status);

    






    void add(const UChar *chars, int32_t count, UErrorCode &status);

    







    const UnicodeString *get(int32_t index) const;

    




    int32_t size() const;

private:
    UnicodeString *strings;
    int32_t listMax;
    int32_t listSize;
};





class StringToCEsMap;
class CEToStringsMap;

















class CollData
{
public:
    





    CollData(UCollator *collator, UErrorCode &status);

    


    ~CollData();

    




    UCollator *getCollator() const;

    









    const StringList *getStringList(int32_t ce) const;

    








    const CEList *getCEList(const UnicodeString *string) const;

    




    void freeCEList(const CEList *list);

    








    int32_t minLengthInChars(const CEList *ces, int32_t offset) const;

 
    
















   int32_t minLengthInChars(const CEList *ces, int32_t offset, int32_t *history) const;

private:
    UCollator      *coll;
    CEToStringsMap *ceToCharsStartingWith;

    uint32_t minHan;
    uint32_t maxHan;

    uint32_t jamoLimits[4];
};

#endif 
#endif 
