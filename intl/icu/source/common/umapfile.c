

















#include "unicode/putil.h"
#include "udatamem.h"
#include "umapfile.h"



#if MAP_IMPLEMENTATION==MAP_WIN32
#   define WIN32_LEAN_AND_MEAN
#   define VC_EXTRALEAN
#   define NOUSER
#   define NOSERVICE
#   define NOIME
#   define NOMCX
#   include <windows.h>
#   include "cmemory.h"

    typedef HANDLE MemoryMap;

#   define IS_MAP(map) ((map)!=NULL)
#elif MAP_IMPLEMENTATION==MAP_POSIX || MAP_IMPLEMENTATION==MAP_390DLL
    typedef size_t MemoryMap;

#   define IS_MAP(map) ((map)!=0)

#   include <unistd.h>
#   include <sys/mman.h>
#   include <sys/stat.h>
#   include <fcntl.h>

#   ifndef MAP_FAILED
#       define MAP_FAILED ((void*)-1)
#   endif

#   if MAP_IMPLEMENTATION==MAP_390DLL
        
#       include <dll.h>
#       include "cstring.h"
#       include "cmemory.h"
#       include "unicode/udata.h"
#       define LIB_PREFIX "lib"
#       define LIB_SUFFIX ".dll"
        
#       define U_ICUDATA_ENTRY_NAME "icudt" U_ICU_VERSION_SHORT U_LIB_SUFFIX_C_NAME_STRING "_dat"
#   endif
#elif MAP_IMPLEMENTATION==MAP_STDIO
#   include <stdio.h>
#   include "cmemory.h"

    typedef void *MemoryMap;

#   define IS_MAP(map) ((map)!=NULL)
#endif







#if MAP_IMPLEMENTATION==MAP_NONE
    U_CFUNC UBool
    uprv_mapFile(UDataMemory *pData, const char *path) {
        UDataMemory_init(pData); 
        return FALSE;            
    }

    U_CFUNC void uprv_unmapFile(UDataMemory *pData) {
        
    }
#elif MAP_IMPLEMENTATION==MAP_WIN32
    U_CFUNC UBool
    uprv_mapFile(
         UDataMemory *pData,    
                                
         const char *path       
         )
    {
        HANDLE map;
        HANDLE file;
        SECURITY_ATTRIBUTES mappingAttributes;
        SECURITY_ATTRIBUTES *mappingAttributesPtr = NULL;
        SECURITY_DESCRIPTOR securityDesc;

        UDataMemory_init(pData); 

        
        file=CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL|FILE_FLAG_RANDOM_ACCESS, NULL);
        if(file==INVALID_HANDLE_VALUE) {
            return FALSE;
        }

        

        if (InitializeSecurityDescriptor(&securityDesc, SECURITY_DESCRIPTOR_REVISION)) {
            
            if (SetSecurityDescriptorDacl(&securityDesc, TRUE, (PACL)NULL, FALSE)) {
                
                uprv_memset(&mappingAttributes, 0, sizeof(mappingAttributes));
                mappingAttributes.nLength = sizeof(mappingAttributes);
                mappingAttributes.lpSecurityDescriptor = &securityDesc;
                mappingAttributes.bInheritHandle = FALSE; 
                mappingAttributesPtr = &mappingAttributes;
            }
        }
        


        
        map=CreateFileMapping(file, mappingAttributesPtr, PAGE_READONLY, 0, 0, NULL);
        CloseHandle(file);
        if(map==NULL) {
            return FALSE;
        }

        
        pData->pHeader=(const DataHeader *)MapViewOfFile(map, FILE_MAP_READ, 0, 0, 0);
        if(pData->pHeader==NULL) {
            CloseHandle(map);
            return FALSE;
        }
        pData->map=map;
        return TRUE;
    }

    U_CFUNC void
    uprv_unmapFile(UDataMemory *pData) {
        if(pData!=NULL && pData->map!=NULL) {
            UnmapViewOfFile(pData->pHeader);
            CloseHandle(pData->map);
            pData->pHeader=NULL;
            pData->map=NULL;
        }
    }



#elif MAP_IMPLEMENTATION==MAP_POSIX
    U_CFUNC UBool
    uprv_mapFile(UDataMemory *pData, const char *path) {
        int fd;
        int length;
        struct stat mystat;
        void *data;

        UDataMemory_init(pData); 

        
        if(stat(path, &mystat)!=0 || mystat.st_size<=0) {
            return FALSE;
        }
        length=mystat.st_size;

        
        fd=open(path, O_RDONLY);
        if(fd==-1) {
            return FALSE;
        }

        
#if U_PLATFORM != U_PF_HPUX
        data=mmap(0, length, PROT_READ, MAP_SHARED,  fd, 0);
#else
        data=mmap(0, length, PROT_READ, MAP_PRIVATE, fd, 0);
#endif
        close(fd); 
        if(data==MAP_FAILED) {
            return FALSE;
        }

        pData->map = (char *)data + length;
        pData->pHeader=(const DataHeader *)data;
        pData->mapAddr = data;
#if U_PLATFORM == U_PF_IPHONE
        posix_madvise(data, length, POSIX_MADV_RANDOM);
#endif
        return TRUE;
    }

    U_CFUNC void
    uprv_unmapFile(UDataMemory *pData) {
        if(pData!=NULL && pData->map!=NULL) {
            size_t dataLen = (char *)pData->map - (char *)pData->mapAddr;
            if(munmap(pData->mapAddr, dataLen)==-1) {
            }
            pData->pHeader=NULL;
            pData->map=0;
            pData->mapAddr=NULL;
        }
    }



#elif MAP_IMPLEMENTATION==MAP_STDIO
    
    static int32_t
    umap_fsize(FILE *f) {
        int32_t savedPos = ftell(f);
        int32_t size = 0;

        

        fseek(f, 0, SEEK_END);
        size = (int32_t)ftell(f);
        fseek(f, savedPos, SEEK_SET);
        return size;
    }

    U_CFUNC UBool
    uprv_mapFile(UDataMemory *pData, const char *path) {
        FILE *file;
        int32_t fileLength;
        void *p;

        UDataMemory_init(pData); 
        
        file=fopen(path, "rb");
        if(file==NULL) {
            return FALSE;
        }

        
        fileLength=umap_fsize(file);
        if(ferror(file) || fileLength<=20) {
            fclose(file);
            return FALSE;
        }

        
        p=uprv_malloc(fileLength);
        if(p==NULL) {
            fclose(file);
            return FALSE;
        }

        
        if(fileLength!=fread(p, 1, fileLength, file)) {
            uprv_free(p);
            fclose(file);
            return FALSE;
        }

        fclose(file);
        pData->map=p;
        pData->pHeader=(const DataHeader *)p;
        pData->mapAddr=p;
        return TRUE;
    }

    U_CFUNC void
    uprv_unmapFile(UDataMemory *pData) {
        if(pData!=NULL && pData->map!=NULL) {
            uprv_free(pData->map);
            pData->map     = NULL;
            pData->mapAddr = NULL;
            pData->pHeader = NULL;
        }
    }


#elif MAP_IMPLEMENTATION==MAP_390DLL
    









    static char *strcpy_returnEnd(char *dest, const char *src)
    {
        while((*dest=*src)!=0) {
            ++dest;
            ++src;
        }
        return dest;
    }
    
    





















    static char *uprv_computeDirPath(const char *path, char *pathBuffer)
    {
        char   *finalSlash;       
        int32_t pathLen;          
        
        finalSlash = 0;
        if (path != 0) {
            finalSlash = uprv_strrchr(path, U_FILE_SEP_CHAR);
        }
        
        *pathBuffer = 0;
        if (finalSlash == 0) {
        

            const char *icuDataDir;
            icuDataDir=u_getDataDirectory();
            if(icuDataDir!=NULL && *icuDataDir!=0) {
                return strcpy_returnEnd(pathBuffer, icuDataDir);
            } else {
                
                return pathBuffer;
            }
        } 
        
        

        pathLen = (int32_t)(finalSlash - path + 1);
        uprv_memcpy(pathBuffer, path, pathLen);
        *(pathBuffer+pathLen) = 0;
        return pathBuffer+pathLen;
    }
    

#   define DATA_TYPE "dat"

    U_CFUNC UBool uprv_mapFile(UDataMemory *pData, const char *path) {
        const char *inBasename;
        char *basename;
        char pathBuffer[1024];
        const DataHeader *pHeader;
        dllhandle *handle;
        void *val=0;

        inBasename=uprv_strrchr(path, U_FILE_SEP_CHAR);
        if(inBasename==NULL) {
            inBasename = path;
        } else {
            inBasename++;
        }
        basename=uprv_computeDirPath(path, pathBuffer);
        if(uprv_strcmp(inBasename, U_ICUDATA_NAME".dat") != 0) {
            
            int fd;
            int length;
            struct stat mystat;
            void *data;
            UDataMemory_init(pData); 

            
            if(stat(path, &mystat)!=0 || mystat.st_size<=0) {
                return FALSE;
            }
            length=mystat.st_size;

            
            fd=open(path, O_RDONLY);
            if(fd==-1) {
                return FALSE;
            }

            
            data=mmap(0, length, PROT_READ, MAP_PRIVATE, fd, 0);
            close(fd); 
            if(data==MAP_FAILED) {
                return FALSE;
            }
            pData->map = (char *)data + length;
            pData->pHeader=(const DataHeader *)data;
            pData->mapAddr = data;
            return TRUE;
        }

#       ifdef OS390BATCH
            




            
            
            
            
            uprv_strcpy(pathBuffer, "IXMI" U_ICU_VERSION_SHORT "DA");
#       else
            
            uprv_strcpy(basename, LIB_PREFIX U_LIBICUDATA_NAME U_ICU_VERSION_SHORT LIB_SUFFIX);
#       endif

#       ifdef UDATA_DEBUG
             fprintf(stderr, "dllload: %s ", pathBuffer);
#       endif

        handle=dllload(pathBuffer);

#       ifdef UDATA_DEBUG
               fprintf(stderr, " -> %08X\n", handle );
#       endif

        if(handle != NULL) {
               
               
               UDataMemory_init(pData); 
               val=dllqueryvar((dllhandle*)handle, U_ICUDATA_ENTRY_NAME);
               if(val == 0) {
                    
                    return FALSE;
               }
#              ifdef UDATA_DEBUG
                    fprintf(stderr, "dllqueryvar(%08X, %s) -> %08X\n", handle, U_ICUDATA_ENTRY_NAME, val);
#              endif

               pData->pHeader=(const DataHeader *)val;
               return TRUE;
         } else {
               return FALSE; 
         }
    }

    U_CFUNC void uprv_unmapFile(UDataMemory *pData) {
        if(pData!=NULL && pData->map!=NULL) {
            uprv_free(pData->map);
            pData->map     = NULL;
            pData->mapAddr = NULL;
            pData->pHeader = NULL;
        }   
    }

#else
#   error MAP_IMPLEMENTATION is set incorrectly
#endif
