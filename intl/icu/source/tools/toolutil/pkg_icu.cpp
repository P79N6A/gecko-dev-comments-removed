




#include "unicode/utypes.h"
#include "unicode/putil.h"
#include "cstring.h"
#include "toolutil.h"
#include "uoptions.h"
#include "uparse.h"
#include "package.h"
#include "pkg_icu.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define LENGTHOF(array) (int32_t)(sizeof(array)/sizeof((array)[0]))



U_NAMESPACE_USE

static const struct {
    const char *suffix;
    int32_t length;
} listFileSuffixes[]={
    { ".txt", 4 },
    { ".lst", 4 },
    { ".tmp", 4 }
};


static UBool
isListTextFile(const char *listname) {
    const char *listNameEnd=strchr(listname, 0);
    const char *suffix;
    int32_t i, length;
    for(i=0; i<LENGTHOF(listFileSuffixes); ++i) {
        suffix=listFileSuffixes[i].suffix;
        length=listFileSuffixes[i].length;
        if((listNameEnd-listname)>length && 0==memcmp(listNameEnd-length, suffix, length)) {
            return TRUE;
        }
    }
    return FALSE;
}








U_CAPI Package * U_EXPORT2
readList(const char *filesPath, const char *listname, UBool readContents, Package *listPkgIn) {
    Package *listPkg = listPkgIn;
    FILE *file;
    const char *listNameEnd;

    if(listname==NULL || listname[0]==0) {
        fprintf(stderr, "missing list file\n");
        return NULL;
    }

    if (listPkg == NULL) {
        listPkg=new Package();
        if(listPkg==NULL) {
            fprintf(stderr, "icupkg: not enough memory\n");
            exit(U_MEMORY_ALLOCATION_ERROR);
        }
    }

    listNameEnd=strchr(listname, 0);
    if(isListTextFile(listname)) {
        
        char line[1024];
        char *end;
        const char *start;

        file=fopen(listname, "r");
        if(file==NULL) {
            fprintf(stderr, "icupkg: unable to open list file \"%s\"\n", listname);
            delete listPkg;
            exit(U_FILE_ACCESS_ERROR);
        }

        while(fgets(line, sizeof(line), file)) {
            
            end=strchr(line, '#');
            if(end!=NULL) {
                *end=0;
            } else {
                
                end=strchr(line, 0);
                while(line<end && (*(end-1)=='\r' || *(end-1)=='\n')) {
                    *--end=0;
                }
            }

            
            
            
            start=u_skipWhitespace(line);
            if(*start==0 || NULL!=strchr(U_PKG_RESERVED_CHARS, *start)) {
                continue;
            }

            
            for(;;) {
                
                for(end=(char *)start; *end!=0 && *end!=' ' && *end!='\t'; ++end) {}
                if(*end==0) {
                    
                    end=NULL;
                } else {
                    
                    *end=0;
                }
                if(readContents) {
                    listPkg->addFile(filesPath, start);
                } else {
                    listPkg->addItem(start);
                }

                
                if(end==NULL || *(start=u_skipWhitespace(end+1))==0) {
                    break;
                }
            }
        }
        fclose(file);
    } else if((listNameEnd-listname)>4 && 0==memcmp(listNameEnd-4, ".dat", 4)) {
        
        listPkg->readPackage(listname);
    } else {
        
        if(readContents) {
            listPkg->addFile(filesPath, listname);
        } else {
            listPkg->addItem(listname);
        }
    }

    return listPkg;
}

U_CAPI int U_EXPORT2
writePackageDatFile(const char *outFilename, const char *outComment, const char *sourcePath, const char *addList, Package *pkg, char outType) {
    Package *addListPkg = NULL;
    UBool pkgDelete = FALSE;

    if (pkg == NULL) {
        pkg = new Package;
        if(pkg == NULL) {
            fprintf(stderr, "icupkg: not enough memory\n");
            return U_MEMORY_ALLOCATION_ERROR;
        }

        addListPkg = readList(sourcePath, addList, TRUE, NULL);
        if(addListPkg != NULL) {
            pkg->addItems(*addListPkg);
        } else {
            return U_ILLEGAL_ARGUMENT_ERROR;
        }

        pkgDelete = TRUE;
    }

    pkg->writePackage(outFilename, outType, outComment);

    if (pkgDelete) {
        delete pkg;
        delete addListPkg;
    }

    return 0;
}

