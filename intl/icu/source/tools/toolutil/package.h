

















#ifndef __PACKAGE_H__
#define __PACKAGE_H__

#include "unicode/utypes.h"

#include <stdio.h>



#define STRING_STORE_SIZE 100000
#define MAX_PKG_NAME_LENGTH 32

typedef void CheckDependency(void *context, const char *itemName, const char *targetName);

U_NAMESPACE_BEGIN

struct Item {
    char *name;
    uint8_t *data;
    int32_t length;
    UBool isDataOwned;
    char type;
};

class U_TOOLUTIL_API Package {
public:
    



    Package();

    
    ~Package();

    




    void readPackage(const char *filename);
    






    void writePackage(const char *filename, char outType, const char *comment);

    


    char getInType();

    
    int32_t findItem(const char *name, int32_t length=-1) const;

    



    void findItems(const char *pattern);
    int32_t findNextItem();
    




    void setMatchMode(uint32_t mode);

    enum {
        MATCH_NOSLASH=1
    };

    void addItem(const char *name);
    void addItem(const char *name, uint8_t *data, int32_t length, UBool isDataOwned, char type);
    void addFile(const char *filesPath, const char *name);
    void addItems(const Package &listPkg);

    void removeItem(int32_t itemIndex);
    void removeItems(const char *pattern);
    void removeItems(const Package &listPkg);

    
    void extractItem(const char *filesPath, int32_t itemIndex, char outType);
    void extractItems(const char *filesPath, const char *pattern, char outType);
    void extractItems(const char *filesPath, const Package &listPkg, char outType);

    
    void extractItem(const char *filesPath, const char *outName, int32_t itemIndex, char outType);

    int32_t getItemCount() const;
    const Item *getItem(int32_t idx) const;

    


    UBool checkDependencies();

    




    void enumDependencies(void *context, CheckDependency check);

private:
    void enumDependencies(Item *pItem, void *context, CheckDependency check);

    


    static void checkDependency(void *context, const char *itemName, const char *targetName);

    



    char *allocString(UBool in, int32_t length);

    void sortItems();

    
    char inPkgName[MAX_PKG_NAME_LENGTH];

    uint8_t *inData;
    uint8_t header[1024];
    int32_t inLength, headerLength;
    uint8_t inCharset;
    UBool inIsBigEndian;

    int32_t itemCount;
    int32_t itemMax;
    Item   *items;

    int32_t inStringTop, outStringTop;
    char inStrings[STRING_STORE_SIZE], outStrings[STRING_STORE_SIZE];

    
    uint32_t matchMode;

    
    const char *findPrefix, *findSuffix;
    int32_t findPrefixLength, findSuffixLength;
    int32_t findNextIndex;

    
    UBool isMissingItems;

    


    void setItemCapacity(int32_t max);

    


    void ensureItemCapacity();
};

U_NAMESPACE_END

#endif


