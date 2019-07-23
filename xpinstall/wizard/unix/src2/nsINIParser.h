






































#ifndef _NS_INIPARSER_H_
#define _NS_INIPARSER_H_

#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <sys/stat.h>
#include <locale.h>

class nsINIParser
{
public:

    






    nsINIParser(char *aFilename);
    ~nsINIParser();

    














    int GetString(      char *aSection, char *aKey, 
                        char *aValBuf, int *aIOValBufSize );

    














    int GetStringAlloc( char *aSection, char *aKey, 
                        char **aOutBuf, int *aOutBufSize );
    
    








    int GetError();

    













    static char     *ResolveName(char *aINIRoot);




    enum 
    {
        OK                  = 0,
        E_READ              = -701,
        E_MEM               = -702,
        E_PARAM             = -703,
        E_NO_SEC            = -704,
        E_NO_KEY            = -705,
        E_SEC_CORRUPT       = -706,
        E_SMALL_BUF         = -707
    };

private:
    int FindSection(char *aSection, char **aOutSecPtr);
    int FindKey(char *aSecPtr, char *aKey, char *aVal, int *aIOValSize);

    char    *mFileBuf;
    int     mFileBufSize;
    int     mError;
};

#define NL '\n'
#define MAX_VAL_SIZE 512

#if defined(DUMP)
#undef DUMP
#endif
#if defined(DEBUG_sgehani) || defined(DEBUG_druidd) || defined(DEBUG_root)
#define DUMP(_msg) printf("%s %d: %s \n", __FILE__, __LINE__, _msg);
#else
#define DUMP(_msg) 
#endif


#endif 
