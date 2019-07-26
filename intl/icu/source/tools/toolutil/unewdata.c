















#include <stdio.h>
#include "unicode/utypes.h"
#include "unicode/putil.h"
#include "unicode/ustring.h"
#include "cmemory.h"
#include "cstring.h"
#include "filestrm.h"
#include "unicode/udata.h"
#include "unewdata.h"

struct UNewDataMemory {
    FileStream *file;
    uint16_t headerSize;
    uint8_t magic1, magic2;
};

U_CAPI UNewDataMemory * U_EXPORT2
udata_create(const char *dir, const char *type, const char *name,
             const UDataInfo *pInfo,
             const char *comment,
             UErrorCode *pErrorCode) {
    UNewDataMemory *pData;
    uint16_t headerSize, commentLength;
    char filename[512];
    uint8_t bytes[16];
    int length;

    if(pErrorCode==NULL || U_FAILURE(*pErrorCode)) {
        return NULL;
    } else if(name==NULL || *name==0 || pInfo==NULL) {
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return NULL;
    }

    
    pData=(UNewDataMemory *)uprv_malloc(sizeof(UNewDataMemory));
    if(pData==NULL) {
        *pErrorCode=U_MEMORY_ALLOCATION_ERROR;
        return NULL;
    }
    
    
    length = 0;					
    if(dir != NULL  && *dir !=0)	
    {
    	length += strlen(dir);
	
    	
        if (dir[strlen(dir) - 1]!= U_FILE_SEP_CHAR) {
            length++;
        }
	}
    length += strlen(name);		

    if(type != NULL  && *type !=0) { 
        length += strlen(type);
    }

        
     
    if(length  > (sizeof(filename) - 1))
    {
   	    *pErrorCode = U_BUFFER_OVERFLOW_ERROR;
   	    uprv_free(pData);
	    return NULL;
    }
   
    
    if(dir!=NULL && *dir!=0) { 
        char *p=filename+strlen(dir);
        uprv_strcpy(filename, dir);
        if (*(p-1)!=U_FILE_SEP_CHAR) {
            *p++=U_FILE_SEP_CHAR;
            *p=0;
        }
    } else { 
        filename[0]=0;
    }
    uprv_strcat(filename, name);
    if(type!=NULL && *type!=0) {
        uprv_strcat(filename, ".");
        uprv_strcat(filename, type);
    }
    pData->file=T_FileStream_open(filename, "wb");
    if(pData->file==NULL) {
        uprv_free(pData);
        *pErrorCode=U_FILE_ACCESS_ERROR;
        return NULL;
    }

    
    headerSize=(uint16_t)(pInfo->size+4);
    if(comment!=NULL && *comment!=0) {
        commentLength=(uint16_t)(uprv_strlen(comment)+1);
        headerSize+=commentLength;
    } else {
        commentLength=0;
    }

    
    pData->headerSize=(uint16_t)((headerSize+15)&~0xf);
    pData->magic1=0xda;
    pData->magic2=0x27;
    T_FileStream_write(pData->file, &pData->headerSize, 4);

    
    T_FileStream_write(pData->file, pInfo, pInfo->size);

    
    if(commentLength>0) {
        T_FileStream_write(pData->file, comment, commentLength);
    }

    
    headerSize&=0xf;
    if(headerSize!=0) {
        headerSize=(uint16_t)(16-headerSize);
        uprv_memset(bytes, 0, headerSize);
        T_FileStream_write(pData->file, bytes, headerSize);
    }

    return pData;
}

U_CAPI uint32_t U_EXPORT2
udata_finish(UNewDataMemory *pData, UErrorCode *pErrorCode) {
    uint32_t fileLength=0;

    if(pErrorCode==NULL || U_FAILURE(*pErrorCode)) {
        return 0;
    }

    if(pData!=NULL) {
        if(pData->file!=NULL) {
            
            fileLength=T_FileStream_size(pData->file);
            if(T_FileStream_error(pData->file)) {
                *pErrorCode=U_FILE_ACCESS_ERROR;
            } else {
                fileLength-=pData->headerSize;
            }
            T_FileStream_close(pData->file);
        }
        uprv_free(pData);
    }

    return fileLength;
}


static const UDataInfo dummyDataInfo = {
    sizeof(UDataInfo),
    0,

    U_IS_BIG_ENDIAN,
    U_CHARSET_FAMILY,
    U_SIZEOF_UCHAR,
    0,

    { 0, 0, 0, 0 },                 
    { 0, 0, 0, 0 },                 
    { 0, 0, 0, 0 }                  
};

U_CAPI void U_EXPORT2
udata_createDummy(const char *dir, const char *type, const char *name, UErrorCode *pErrorCode) {
    if(U_SUCCESS(*pErrorCode)) {
        udata_finish(udata_create(dir, type, name, &dummyDataInfo, NULL, pErrorCode), pErrorCode);
        if(U_FAILURE(*pErrorCode)) {
            fprintf(stderr, "error %s writing dummy data file %s" U_FILE_SEP_STRING "%s.%s\n",
                    u_errorName(*pErrorCode), dir, name, type);
            exit(*pErrorCode);
        }
    }
}

U_CAPI void U_EXPORT2
udata_write8(UNewDataMemory *pData, uint8_t byte) {
    if(pData!=NULL && pData->file!=NULL) {
        T_FileStream_write(pData->file, &byte, 1);
    }
}

U_CAPI void U_EXPORT2
udata_write16(UNewDataMemory *pData, uint16_t word) {
    if(pData!=NULL && pData->file!=NULL) {
        T_FileStream_write(pData->file, &word, 2);
    }
}

U_CAPI void U_EXPORT2
udata_write32(UNewDataMemory *pData, uint32_t wyde) {
    if(pData!=NULL && pData->file!=NULL) {
        T_FileStream_write(pData->file, &wyde, 4);
    }
}

U_CAPI void U_EXPORT2
udata_writeBlock(UNewDataMemory *pData, const void *s, int32_t length) {
    if(pData!=NULL && pData->file!=NULL) {
        if(length>0) {
            T_FileStream_write(pData->file, s, length);
        }
    }
}

U_CAPI void U_EXPORT2
udata_writePadding(UNewDataMemory *pData, int32_t length) {
    static const uint8_t padding[16]={
        0xaa, 0xaa, 0xaa, 0xaa,
        0xaa, 0xaa, 0xaa, 0xaa,
        0xaa, 0xaa, 0xaa, 0xaa,
        0xaa, 0xaa, 0xaa, 0xaa
    };
    if(pData!=NULL && pData->file!=NULL) {
        while(length>=16) {
            T_FileStream_write(pData->file, padding, 16);
            length-=16;
        }
        if(length>0) {
            T_FileStream_write(pData->file, padding, length);
        }
    }
}

U_CAPI void U_EXPORT2
udata_writeString(UNewDataMemory *pData, const char *s, int32_t length) {
    if(pData!=NULL && pData->file!=NULL) {
        if(length==-1) {
            length=(int32_t)uprv_strlen(s);
        }
        if(length>0) {
            T_FileStream_write(pData->file, s, length);
        }
    }
}

U_CAPI void U_EXPORT2
udata_writeUString(UNewDataMemory *pData, const UChar *s, int32_t length) {
    if(pData!=NULL && pData->file!=NULL) {
        if(length==-1) {
            length=u_strlen(s);
        }
        if(length>0) {
            T_FileStream_write(pData->file, s, length*sizeof(UChar));
        }
    }
}










