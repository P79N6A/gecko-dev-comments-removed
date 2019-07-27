










#ifndef SkOSFile_DEFINED
#define SkOSFile_DEFINED

#include "SkString.h"

#if defined(SK_BUILD_FOR_MAC) || defined(SK_BUILD_FOR_UNIX) || defined(SK_BUILD_FOR_ANDROID) || defined(SK_BUILD_FOR_IOS)
    #include <dirent.h>
#endif

#include <stddef.h> 

struct SkFILE;

enum SkFILE_Flags {
    kRead_SkFILE_Flag   = 0x01,
    kWrite_SkFILE_Flag  = 0x02
};

#ifdef _WIN32
const static char SkPATH_SEPARATOR = '\\';
#else
const static char SkPATH_SEPARATOR = '/';
#endif

SkFILE* sk_fopen(const char path[], SkFILE_Flags);
void    sk_fclose(SkFILE*);

size_t  sk_fgetsize(SkFILE*);


bool    sk_frewind(SkFILE*);

size_t  sk_fread(void* buffer, size_t byteCount, SkFILE*);
size_t  sk_fwrite(const void* buffer, size_t byteCount, SkFILE*);

char*   sk_fgets(char* str, int size, SkFILE* f);

void    sk_fflush(SkFILE*);

bool    sk_fseek(SkFILE*, size_t);
bool    sk_fmove(SkFILE*, long);
size_t  sk_ftell(SkFILE*);





void*   sk_fmmap(SkFILE* f, size_t* length);





void*   sk_fdmmap(int fd, size_t* length);




void    sk_fmunmap(const void* addr, size_t length);


bool    sk_fidentical(SkFILE* a, SkFILE* b);




int     sk_fileno(SkFILE* f);




bool    sk_exists(const char *path, SkFILE_Flags = (SkFILE_Flags)0);


bool    sk_isdir(const char *path);


int sk_feof(SkFILE *);





bool    sk_mkdir(const char* path);

class SkOSFile {
public:
    class Iter {
    public:
        Iter();
        Iter(const char path[], const char suffix[] = NULL);
        ~Iter();

        void reset(const char path[], const char suffix[] = NULL);
        



        bool next(SkString* name, bool getDir = false);

    private:
#ifdef SK_BUILD_FOR_WIN
        HANDLE      fHandle;
        uint16_t*   fPath16;
#elif defined(SK_BUILD_FOR_MAC) || defined(SK_BUILD_FOR_UNIX) || defined(SK_BUILD_FOR_ANDROID) || defined(SK_BUILD_FOR_IOS)
        DIR*        fDIR;
        SkString    fPath, fSuffix;
#endif
    };
};




class SkOSPath {
public:
    







    static SkString SkPathJoin(const char *rootPath, const char *relativePath);

    







    static SkString SkBasename(const char* fullPath);
};
#endif
