























#include "unicode/utypes.h"
#include "unicode/putil.h"
#include "unicode/udata.h"
#include "cmemory.h"
#include "cstring.h"
#include "uinvchar.h"
#include "uarrsort.h"
#include "ucmndata.h"
#include "udataswp.h"
#include "swapimpl.h"
#include "toolutil.h"
#include "uoptions.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>



#define LENGTHOF(array) (int32_t)(sizeof(array)/sizeof((array)[0]))
#define DEFAULT_PADDING_LENGTH 15

static UOption options[]={
    UOPTION_HELP_H,
    UOPTION_HELP_QUESTION_MARK,
    UOPTION_DEF("type", 't', UOPT_REQUIRES_ARG)
};

enum {
    OPT_HELP_H,
    OPT_HELP_QUESTION_MARK,
    OPT_OUT_TYPE
};

static int32_t
fileSize(FILE *f) {
    int32_t size;

    fseek(f, 0, SEEK_END);
    size=(int32_t)ftell(f);
    fseek(f, 0, SEEK_SET);
    return size;
}




U_CFUNC int32_t U_CALLCONV
udata_swapPackage(const char *inFilename, const char *outFilename,
                  const UDataSwapper *ds,
                  const void *inData, int32_t length, void *outData,
                  UErrorCode *pErrorCode);

U_CDECL_BEGIN
static void U_CALLCONV
printError(void *context, const char *fmt, va_list args) {
    vfprintf((FILE *)context, fmt, args);
}
U_CDECL_END

static int
printUsage(const char *pname, UBool ishelp) {
    fprintf(stderr,
            "%csage: %s [ -h, -?, --help ] -tl|-tb|-te|--type=b|... infilename outfilename\n",
            ishelp ? 'U' : 'u', pname);
    if(ishelp) {
        fprintf(stderr,
              "\nOptions: -h, -?, --help    print this message and exit\n"
                "         Read the input file, swap its platform properties according\n"
                "         to the -t or --type option, and write the result to the output file.\n"
                "         -tl               change to little-endian/ASCII charset family\n"
                "         -tb               change to big-endian/ASCII charset family\n"
                "         -te               change to big-endian/EBCDIC charset family\n");
    }

    return !ishelp;
}

extern int
main(int argc, char *argv[]) {
    FILE *in, *out;
    const char *pname;
    char *data;
    int32_t length;
    UBool ishelp;
    int rc;

    UDataSwapper *ds;
    const UDataInfo *pInfo;
    UErrorCode errorCode;
    uint8_t outCharset;
    UBool outIsBigEndian;

    U_MAIN_INIT_ARGS(argc, argv);

    fprintf(stderr, "Warning: icuswap is an obsolete tool and it will be removed in the next ICU release.\nPlease use the icupkg tool instead.\n");

    
    pname=strrchr(argv[0], U_FILE_SEP_CHAR);
    if(pname==NULL) {
        pname=strrchr(argv[0], '/');
    }
    if(pname!=NULL) {
        ++pname;
    } else {
        pname=argv[0];
    }

    argc=u_parseArgs(argc, argv, LENGTHOF(options), options);
    ishelp=options[OPT_HELP_H].doesOccur || options[OPT_HELP_QUESTION_MARK].doesOccur;
    if(ishelp || argc!=3) {
        return printUsage(pname, ishelp);
    }

    
    data=(char *)options[OPT_OUT_TYPE].value;
    if(data[0]==0 || data[1]!=0) {
        
        return printUsage(pname, FALSE);
    }
    switch(data[0]) {
    case 'l':
        outIsBigEndian=FALSE;
        outCharset=U_ASCII_FAMILY;
        break;
    case 'b':
        outIsBigEndian=TRUE;
        outCharset=U_ASCII_FAMILY;
        break;
    case 'e':
        outIsBigEndian=TRUE;
        outCharset=U_EBCDIC_FAMILY;
        break;
    default:
        return printUsage(pname, FALSE);
    }

    in=out=NULL;
    data=NULL;

    
    in=fopen(argv[1], "rb");
    if(in==NULL) {
        fprintf(stderr, "%s: unable to open input file \"%s\"\n", pname, argv[1]);
        rc=2;
        goto done;
    }

    length=fileSize(in);
    if(length<DEFAULT_PADDING_LENGTH) {
        fprintf(stderr, "%s: empty input file \"%s\"\n", pname, argv[1]);
        rc=2;
        goto done;
    }

    





    data=(char *)malloc(length+DEFAULT_PADDING_LENGTH);
    if(data==NULL) {
        fprintf(stderr, "%s: error allocating memory for \"%s\"\n", pname, argv[1]);
        rc=2;
        goto done;
    }

    
    uprv_memset(data+length-DEFAULT_PADDING_LENGTH, 0xaa, DEFAULT_PADDING_LENGTH);

    if(length!=(int32_t)fread(data, 1, length, in)) {
        fprintf(stderr, "%s: error reading \"%s\"\n", pname, argv[1]);
        rc=3;
        goto done;
    }

    fclose(in);
    in=NULL;

    
    errorCode=U_ZERO_ERROR;
    ds=udata_openSwapperForInputData(data, length, outIsBigEndian, outCharset, &errorCode);
    if(U_FAILURE(errorCode)) {
        fprintf(stderr, "%s: udata_openSwapperForInputData(\"%s\") failed - %s\n",
                pname, argv[1], u_errorName(errorCode));
        rc=4;
        goto done;
    }

    ds->printError=printError;
    ds->printErrorContext=stderr;

    
    pInfo=(const UDataInfo *)((const char *)data+4);

    if( length>=20 &&
        pInfo->dataFormat[0]==0x43 &&   
        pInfo->dataFormat[1]==0x6d &&
        pInfo->dataFormat[2]==0x6e &&
        pInfo->dataFormat[3]==0x44
    ) {
        






        length=udata_swapPackage(argv[1], argv[2], ds, data, length, data, &errorCode);
        udata_closeSwapper(ds);
        if(U_FAILURE(errorCode)) {
            fprintf(stderr, "%s: udata_swapPackage(\"%s\") failed - %s\n",
                    pname, argv[1], u_errorName(errorCode));
            rc=4;
            goto done;
        }
    } else {
        
        length=udata_swap(ds, data, length, data, &errorCode);
        udata_closeSwapper(ds);
        if(U_FAILURE(errorCode)) {
            fprintf(stderr, "%s: udata_swap(\"%s\") failed - %s\n",
                    pname, argv[1], u_errorName(errorCode));
            rc=4;
            goto done;
        }
    }

    out=fopen(argv[2], "wb");
    if(out==NULL) {
        fprintf(stderr, "%s: unable to open output file \"%s\"\n", pname, argv[2]);
        rc=5;
        goto done;
    }

    if(length!=(int32_t)fwrite(data, 1, length, out)) {
        fprintf(stderr, "%s: error writing \"%s\"\n", pname, argv[2]);
        rc=6;
        goto done;
    }

    fclose(out);
    out=NULL;

    
    rc=0;

done:
    if(in!=NULL) {
        fclose(in);
    }
    if(out!=NULL) {
        fclose(out);
    }
    if(data!=NULL) {
        free(data);
    }
    return rc;
}



static int32_t
extractPackageName(const UDataSwapper *ds, const char *filename,
                   char pkg[], int32_t capacity,
                   UErrorCode *pErrorCode) {
    const char *basename;
    int32_t len;

    if(U_FAILURE(*pErrorCode)) {
        return 0;
    }

    basename=findBasename(filename);
    len=(int32_t)uprv_strlen(basename)-4; 

    if(len<=0 || 0!=uprv_strcmp(basename+len, ".dat")) {
        udata_printError(ds, "udata_swapPackage(): \"%s\" is not recognized as a package filename (must end with .dat)\n",
                         basename);
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

    if(len>=capacity) {
        udata_printError(ds, "udata_swapPackage(): the package name \"%s\" is too long (>=%ld)\n",
                         (long)capacity);
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

    uprv_memcpy(pkg, basename, len);
    pkg[len]=0;
    return len;
}

struct ToCEntry {
    uint32_t nameOffset, inOffset, outOffset, length;
};

U_CDECL_BEGIN
static int32_t U_CALLCONV
compareToCEntries(const void *context, const void *left, const void *right) {
    const char *chars=(const char *)context;
    return (int32_t)uprv_strcmp(chars+((const ToCEntry *)left)->nameOffset,
                                chars+((const ToCEntry *)right)->nameOffset);
}
U_CDECL_END

U_CFUNC int32_t U_CALLCONV
udata_swapPackage(const char *inFilename, const char *outFilename,
                  const UDataSwapper *ds,
                  const void *inData, int32_t length, void *outData,
                  UErrorCode *pErrorCode) {
    const UDataInfo *pInfo;
    int32_t headerSize;

    const uint8_t *inBytes;
    uint8_t *outBytes;

    uint32_t itemCount, offset, i;
    int32_t itemLength;

    const UDataOffsetTOCEntry *inEntries;
    UDataOffsetTOCEntry *outEntries;

    ToCEntry *table;

    char inPkgName[32], outPkgName[32];
    int32_t inPkgNameLength, outPkgNameLength;

    
    headerSize=udata_swapDataHeader(ds, inData, length, outData, pErrorCode);
    if(pErrorCode==NULL || U_FAILURE(*pErrorCode)) {
        return 0;
    }

    
    pInfo=(const UDataInfo *)((const char *)inData+4);
    if(!(
        pInfo->dataFormat[0]==0x43 &&   
        pInfo->dataFormat[1]==0x6d &&
        pInfo->dataFormat[2]==0x6e &&
        pInfo->dataFormat[3]==0x44 &&
        pInfo->formatVersion[0]==1
    )) {
        udata_printError(ds, "udata_swapPackage(): data format %02x.%02x.%02x.%02x (format version %02x) is not recognized as an ICU .dat package\n",
                         pInfo->dataFormat[0], pInfo->dataFormat[1],
                         pInfo->dataFormat[2], pInfo->dataFormat[3],
                         pInfo->formatVersion[0]);
        *pErrorCode=U_UNSUPPORTED_ERROR;
        return 0;
    }

    




    inPkgNameLength=extractPackageName(
                        ds, inFilename,
                        inPkgName, (int32_t)sizeof(inPkgName),
                        pErrorCode);
    outPkgNameLength=extractPackageName(
                        ds, outFilename,
                        outPkgName, (int32_t)sizeof(outPkgName),
                        pErrorCode);
    if(U_FAILURE(*pErrorCode)) {
        return 0;
    }

    




    if(inPkgNameLength!=outPkgNameLength) {
        udata_printError(ds, "udata_swapPackage(): the package names \"%s\" and \"%s\" must have the same length\n",
                         inPkgName, outPkgName);
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

    inBytes=(const uint8_t *)inData+headerSize;
    inEntries=(const UDataOffsetTOCEntry *)(inBytes+4);

    if(length<0) {
        
        itemCount=ds->readUInt32(*(const uint32_t *)inBytes);
        if(itemCount==0) {
            
            return headerSize+4;
        }

        
        offset=ds->readUInt32(inEntries[itemCount-1].dataOffset);
        itemLength=udata_swap(ds, inBytes+offset, -1, NULL, pErrorCode);

        if(U_SUCCESS(*pErrorCode)) {
            return headerSize+offset+(uint32_t)itemLength;
        } else {
            return 0;
        }
    } else {
        
        length-=headerSize;
        if(length<4) {
            
            offset=0xffffffff;
            itemCount=0; 
        } else {
            itemCount=ds->readUInt32(*(const uint32_t *)inBytes);
            if(itemCount==0) {
                offset=4;
            } else if((uint32_t)length<(4+8*itemCount)) {
                
                offset=0xffffffff;
            } else {
                
                offset=20+ds->readUInt32(inEntries[itemCount-1].dataOffset);
            }
        }
        if((uint32_t)length<offset) {
            udata_printError(ds, "udata_swapPackage(): too few bytes (%d after header) for a .dat package\n",
                             length);
            *pErrorCode=U_INDEX_OUTOFBOUNDS_ERROR;
            return 0;
        }

        outBytes=(uint8_t *)outData+headerSize;

        
        ds->swapArray32(ds, inBytes, 4, outBytes, pErrorCode);

        if(itemCount==0) {
            
            return headerSize+4;
        }

        
        offset=4+8*itemCount;
        itemLength=(int32_t)(ds->readUInt32(inEntries[0].dataOffset)-offset);
        udata_swapInvStringBlock(ds, inBytes+offset, itemLength, outBytes+offset, pErrorCode);
        if(U_FAILURE(*pErrorCode)) {
            udata_printError(ds, "udata_swapPackage() failed to swap the data item name strings\n");
            return 0;
        }
        

        
        if(ds->outCharset!=U_CHARSET_FAMILY) {
            UDataSwapper *ds2;
            ds2=udata_openSwapper(TRUE, U_CHARSET_FAMILY, TRUE, ds->outCharset, pErrorCode);
            ds2->swapInvChars(ds2, inPkgName, inPkgNameLength, inPkgName, pErrorCode);
            ds2->swapInvChars(ds2, outPkgName, outPkgNameLength, outPkgName, pErrorCode);
            udata_closeSwapper(ds2);
            if(U_FAILURE(*pErrorCode)) {
                udata_printError(ds, "udata_swapPackage() failed to swap the input/output package names\n");
            }
        }

        
        {
            char *entryName;

            for(i=0; i<itemCount; ++i) {
                entryName=(char *)inBytes+ds->readUInt32(inEntries[i].nameOffset);

                if(0==uprv_memcmp(entryName, inPkgName, inPkgNameLength)) {
                    uprv_memcpy(entryName, outPkgName, inPkgNameLength);
                } else {
                    udata_printError(ds, "udata_swapPackage() failed: ToC item %ld does not have the input package name as a prefix\n",
                                     (long)i);
                    *pErrorCode=U_INVALID_FORMAT_ERROR;
                    return 0;
                }
            }
        }

        
















        if(inData==outData) {
            
            table=(ToCEntry *)uprv_malloc(itemCount*sizeof(ToCEntry)+length+DEFAULT_PADDING_LENGTH);
            if(table!=NULL) {
                outBytes=(uint8_t *)(table+itemCount);

                
                uprv_memcpy(outBytes, inBytes, 4);
                uprv_memcpy(outBytes+offset, inBytes+offset, itemLength);
            }
        } else {
            table=(ToCEntry *)uprv_malloc(itemCount*sizeof(ToCEntry));
        }
        if(table==NULL) {
            udata_printError(ds, "udata_swapPackage(): out of memory allocating %d bytes\n",
                             inData==outData ?
                                 itemCount*sizeof(ToCEntry)+length+DEFAULT_PADDING_LENGTH :
                                 itemCount*sizeof(ToCEntry));
            *pErrorCode=U_MEMORY_ALLOCATION_ERROR;
            return 0;
        }
        outEntries=(UDataOffsetTOCEntry *)(outBytes+4);

        
        for(i=0; i<itemCount; ++i) {
            table[i].nameOffset=ds->readUInt32(inEntries[i].nameOffset);
            table[i].inOffset=ds->readUInt32(inEntries[i].dataOffset);
            if(i>0) {
                table[i-1].length=table[i].inOffset-table[i-1].inOffset;
            }
        }
        table[itemCount-1].length=(uint32_t)length-table[itemCount-1].inOffset;

        if(ds->inCharset==ds->outCharset) {
            
            for(i=0; i<itemCount; ++i) {
                table[i].outOffset=table[i].inOffset;
            }
        } else {
            

            










            if((length&0xf)!=0) {
                int32_t delta=16-(length&0xf);
                length+=delta;
                table[itemCount-1].length+=(uint32_t)delta;
            }

            
            offset=table[0].inOffset;
            
            uprv_sortArray(table, (int32_t)itemCount, (int32_t)sizeof(ToCEntry),
                           compareToCEntries, outBytes, FALSE, pErrorCode);

            




            
            for(i=0; i<itemCount; ++i) {
                table[i].outOffset=offset;
                offset+=table[i].length;
            }
        }

        
        for(i=0; i<itemCount; ++i) {
            ds->writeUInt32(&outEntries[i].nameOffset, table[i].nameOffset);
            ds->writeUInt32(&outEntries[i].dataOffset, table[i].outOffset);
        }

        
        for(i=0; i<itemCount; ++i) {
             
            uprv_memcpy(outBytes+table[i].outOffset, inBytes+table[i].inOffset, table[i].length);

            
            udata_swap(ds, inBytes+table[i].inOffset, (int32_t)table[i].length,
                          outBytes+table[i].outOffset, pErrorCode);

            if(U_FAILURE(*pErrorCode)) {
                if(ds->outCharset==U_CHARSET_FAMILY) {
                    udata_printError(ds, "warning: udata_swapPackage() failed to swap item \"%s\"\n"
                                         "    at inOffset 0x%x length 0x%x - %s\n"
                                         "    the data item will be copied, not swapped\n\n",
                                     (char *)outBytes+table[i].nameOffset,
                                     table[i].inOffset, table[i].length, u_errorName(*pErrorCode));
                } else {
                    udata_printError(ds, "warning: udata_swapPackage() failed to swap an item\n"
                                         "    at inOffset 0x%x length 0x%x - %s\n"
                                         "    the data item will be copied, not swapped\n\n",
                                     table[i].inOffset, table[i].length, u_errorName(*pErrorCode));
                }
                
                *pErrorCode=U_ZERO_ERROR;
                uprv_memcpy(outBytes+table[i].outOffset, inBytes+table[i].inOffset, table[i].length);
            }
        }

        if(inData==outData) {
            
            uprv_memcpy((uint8_t *)outData+headerSize, outBytes, length);
        }
        uprv_free(table);

        return headerSize+length;
    }
}









