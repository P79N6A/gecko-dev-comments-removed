









































#ifndef nsINIParser_h__
#define nsINIParser_h__

#ifdef MOZILLA_INTERNAL_API
#define nsINIParser nsINIParser_internal
#endif

#include "nscore.h"
#include "nsClassHashtable.h"
#include "nsAutoPtr.h"

#include <stdio.h>

class nsILocalFile;

class NS_COM_GLUE nsINIParser
{
public:
    nsINIParser() { }
    ~nsINIParser() { }

    





    nsresult Init(nsILocalFile* aFile);

    





    nsresult Init(const char *aPath);

    



    typedef bool
    (* INISectionCallback)(const char *aSection, void *aClosure);

    


    nsresult GetSections(INISectionCallback aCB, void *aClosure);

    



    typedef bool
    (* INIStringCallback)(const char *aString, const char *aValue,
                          void *aClosure);

    



    nsresult GetStrings(const char *aSection,
                        INIStringCallback aCB, void *aClosure);

    









    nsresult GetString(const char *aSection, const char *aKey, 
                       nsACString &aResult);

    










    nsresult GetString(const char *aSection, const char* aKey,
                       char *aResult, PRUint32 aResultLen);

private:
    struct INIValue
    {
        INIValue(const char *aKey, const char *aValue)
            : key(aKey), value(aValue) { }

        const char *key;
        const char *value;
        nsAutoPtr<INIValue> next;
    };

    struct GSClosureStruct
    {
        INISectionCallback  usercb;
        void               *userclosure;
    };

    nsClassHashtable<nsDepCharHashKey, INIValue> mSections;
    nsAutoArrayPtr<char> mFileContents;    

    nsresult InitFromFILE(FILE *fd);

    static PLDHashOperator GetSectionsCB(const char *aKey,
                                         INIValue *aData, void *aClosure);
};

#endif 
