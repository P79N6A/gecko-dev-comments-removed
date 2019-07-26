



















#include <unicode/utypes.h>
#include <unicode/putil.h>
#include <unicode/ucnv.h>
#include <unicode/uenum.h>
#include <unicode/unistr.h>
#include <unicode/translit.h>
#include <unicode/uset.h>
#include <unicode/uclean.h>
#include <unicode/utf16.h>

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include "cmemory.h"
#include "cstring.h"
#include "ustrfmt.h"

#include "unicode/uwmsg.h"

U_NAMESPACE_USE

#if U_PLATFORM_USES_ONLY_WIN32_API && !defined(__STRICT_ANSI__)
#include <io.h>
#include <fcntl.h>
#if U_PLATFORM_USES_ONLY_WIN32_API
#define USE_FILENO_BINARY_MODE 1

#ifndef fileno
#define fileno _fileno
#endif
#ifndef setmode
#define setmode _setmode
#endif
#ifndef O_BINARY
#define O_BINARY _O_BINARY
#endif
#endif
#endif

#ifdef UCONVMSG_LINK

#include "unicode/utypes.h"
#include "unicode/udata.h"
U_CFUNC char uconvmsg_dat[];
#endif

#define LENGTHOF(array) (int32_t)(sizeof(array)/sizeof((array)[0]))

#define DEFAULT_BUFSZ   4096
#define UCONVMSG "uconvmsg"

static UResourceBundle *gBundle = 0;    







static void initMsg(const char *pname) {
    static int ps = 0;

    if (!ps) {
        char dataPath[2048];        
        UErrorCode err = U_ZERO_ERROR;

        ps = 1;

        
#ifdef UCONVMSG_LINK
        udata_setAppData(UCONVMSG, (const void*) uconvmsg_dat, &err);
        if (U_FAILURE(err)) {
          fprintf(stderr, "%s: warning, problem installing our static resource bundle data uconvmsg: %s - trying anyways.\n",
                  pname, u_errorName(err));
          err = U_ZERO_ERROR; 
        }
#endif

        
        gBundle = u_wmsg_setPath(UCONVMSG, &err);
        if (U_FAILURE(err)) {
            fprintf(stderr,
                    "%s: warning: couldn't open bundle %s: %s\n",
                    pname, UCONVMSG, u_errorName(err));
#ifdef UCONVMSG_LINK
            fprintf(stderr,
                    "%s: setAppData was called, internal data %s failed to load\n",
                        pname, UCONVMSG);
#endif
 
            err = U_ZERO_ERROR;
            
            uprv_strcpy(dataPath, u_getDataDirectory());
            uprv_strcat(dataPath, U_FILE_SEP_STRING);
            uprv_strcat(dataPath, UCONVMSG);

            gBundle = u_wmsg_setPath(dataPath, &err);
            if (U_FAILURE(err)) {
                fprintf(stderr,
                    "%s: warning: still couldn't open bundle %s: %s\n",
                    pname, dataPath, u_errorName(err));
                fprintf(stderr, "%s: warning: messages will not be displayed\n", pname);
            }
        }
    }
}




static struct callback_ent {
    const char *name;
    UConverterFromUCallback fromu;
    const void *fromuctxt;
    UConverterToUCallback tou;
    const void *touctxt;
} transcode_callbacks[] = {
    { "substitute",
      UCNV_FROM_U_CALLBACK_SUBSTITUTE, 0,
      UCNV_TO_U_CALLBACK_SUBSTITUTE, 0 },
    { "skip",
      UCNV_FROM_U_CALLBACK_SKIP, 0,
      UCNV_TO_U_CALLBACK_SKIP, 0 },
    { "stop",
      UCNV_FROM_U_CALLBACK_STOP, 0,
      UCNV_TO_U_CALLBACK_STOP, 0 },
    { "escape",
      UCNV_FROM_U_CALLBACK_ESCAPE, 0,
      UCNV_TO_U_CALLBACK_ESCAPE, 0},
    { "escape-icu",
      UCNV_FROM_U_CALLBACK_ESCAPE, UCNV_ESCAPE_ICU,
      UCNV_TO_U_CALLBACK_ESCAPE, UCNV_ESCAPE_ICU },
    { "escape-java",
      UCNV_FROM_U_CALLBACK_ESCAPE, UCNV_ESCAPE_JAVA,
      UCNV_TO_U_CALLBACK_ESCAPE, UCNV_ESCAPE_JAVA },
    { "escape-c",
      UCNV_FROM_U_CALLBACK_ESCAPE, UCNV_ESCAPE_C,
      UCNV_TO_U_CALLBACK_ESCAPE, UCNV_ESCAPE_C },
    { "escape-xml",
      UCNV_FROM_U_CALLBACK_ESCAPE, UCNV_ESCAPE_XML_HEX,
      UCNV_TO_U_CALLBACK_ESCAPE, UCNV_ESCAPE_XML_HEX },
    { "escape-xml-hex",
      UCNV_FROM_U_CALLBACK_ESCAPE, UCNV_ESCAPE_XML_HEX,
      UCNV_TO_U_CALLBACK_ESCAPE, UCNV_ESCAPE_XML_HEX },
    { "escape-xml-dec",
      UCNV_FROM_U_CALLBACK_ESCAPE, UCNV_ESCAPE_XML_DEC,
      UCNV_TO_U_CALLBACK_ESCAPE, UCNV_ESCAPE_XML_DEC },
    { "escape-unicode", UCNV_FROM_U_CALLBACK_ESCAPE, UCNV_ESCAPE_UNICODE,
      UCNV_TO_U_CALLBACK_ESCAPE, UCNV_ESCAPE_UNICODE }
};



static const struct callback_ent *findCallback(const char *name) {
    int i, count =
        sizeof(transcode_callbacks) / sizeof(*transcode_callbacks);

    


    for (i = 0; i < count; ++i) {
        if (!uprv_stricmp(name, transcode_callbacks[i].name)) {
            return &transcode_callbacks[i];
        }
    }

    return 0;
}






static int printConverters(const char *pname, const char *lookfor,
    UBool canon)
{
    UErrorCode err = U_ZERO_ERROR;
    int32_t num;
    uint16_t num_stds;
    const char **stds;

    

    if (lookfor) {
        if (!canon) {
            printf("%s\n", lookfor);
            return 0;
        } else {
        








            const char *truename = ucnv_getAlias(lookfor, 0, &err);
            if (U_SUCCESS(err)) {
                lookfor = truename;
            } else {
                err = U_ZERO_ERROR;
            }
        }
    }

    




    num = ucnv_countAvailable();
    if (num <= 0) {
        initMsg(pname);
        u_wmsg(stderr, "cantGetNames");
        return -1;
    }
    if (lookfor) {
        num = 1;                
    }

    num_stds = ucnv_countStandards();
    stds = (const char **) uprv_malloc(num_stds * sizeof(*stds));
    if (!stds) {
        u_wmsg(stderr, "cantGetTag", u_wmsg_errorName(U_MEMORY_ALLOCATION_ERROR));
        return -1;
    } else {
        uint16_t s;

        if (canon) {
            printf("{ ");
        }
        for (s = 0; s < num_stds; ++s) {
            stds[s] = ucnv_getStandard(s, &err);
            if (canon) {
                printf("%s ", stds[s]);
            }
            if (U_FAILURE(err)) {
                u_wmsg(stderr, "cantGetTag", u_wmsg_errorName(err));
                goto error_cleanup;
            }
        }
        if (canon) {
            puts("}");
        }
    }

    for (int32_t i = 0; i < num; i++) {
        const char *name;
        uint16_t num_aliases;

        


        if (lookfor) {
            name = lookfor;
        } else {
            name = ucnv_getAvailableName(i);
        }

        

        err = U_ZERO_ERROR;
        num_aliases = ucnv_countAliases(name, &err);
        if (U_FAILURE(err)) {
            printf("%s", name);

            UnicodeString str(name, "");
            putchar('\t');
            u_wmsg(stderr, "cantGetAliases", str.getTerminatedBuffer(),
                u_wmsg_errorName(err));
            goto error_cleanup;
        } else {
            uint16_t a, s, t;

            

            for (a = 0; a < num_aliases; ++a) {
                const char *alias = ucnv_getAlias(name, a, &err);

                if (U_FAILURE(err)) {
                    UnicodeString str(name, "");
                    putchar('\t');
                    u_wmsg(stderr, "cantGetAliases", str.getTerminatedBuffer(),
                        u_wmsg_errorName(err));
                    goto error_cleanup;
                }

                
                printf("%s%s%s", (canon ? (a == 0? "" : "\t" ) : "") ,
                                 alias,
                                 (canon ? "" : " "));

                

                if (canon) {
                    
                    for (s = t = 0; s < num_stds-1; ++s) {
                        UEnumeration *nameEnum = ucnv_openStandardNames(name, stds[s], &err);
                        if (U_SUCCESS(err)) {
                            
                            const char *standardName;
                            UBool isFirst = TRUE;
                            UErrorCode enumError = U_ZERO_ERROR;
                            while ((standardName = uenum_next(nameEnum, NULL, &enumError))) {
                                
                                if (!strcmp(standardName, alias)) {
                                    if (!t) {
                                        printf(" {");
                                        t = 1;
                                    }
                                    
                                    printf(" %s%s", stds[s], (isFirst ? "*" : ""));
                                }
                                isFirst = FALSE;
                            }
                        }
                    }
                    if (t) {
                        printf(" }");
                    }
                }
                
                if (canon) {
                    puts("");
                }

                
            }
            
            if (!canon) {
                puts("");
            }
        }
    }

    

    uprv_free(stds);

    

    return 0;
error_cleanup:
    uprv_free(stds);
    return -1;
}




static int printTransliterators(UBool canon)
{
#if UCONFIG_NO_TRANSLITERATION
    printf("no transliterators available because of UCONFIG_NO_TRANSLITERATION, see uconfig.h\n");
    return 1;
#else
    UErrorCode status = U_ZERO_ERROR;
    UEnumeration *ids = utrans_openIDs(&status);
    int32_t i, numtrans = uenum_count(ids, &status);

    char sepchar = canon ? '\n' : ' ';

    for (i = 0; U_SUCCESS(status)&& (i < numtrans); ++i) {
    	int32_t len;
    	const char *nextTrans = uenum_next(ids, &len, &status);

        printf("%s", nextTrans);
        if (i < numtrans - 1) {
            putchar(sepchar);
        }
    }

    uenum_close(ids);

    

    if (sepchar != '\n') {
        putchar('\n');
    }

    

    return 0;
#endif
}

enum {
    uSP = 0x20,         
    uCR = 0xd,          
    uLF = 0xa,          
    uNL = 0x85,         
    uLS = 0x2028,       
    uPS = 0x2029,       
    uSig = 0xfeff       
};

static inline int32_t
getChunkLimit(const UnicodeString &prev, const UnicodeString &s) {
    
    
    
    
    
    
    
    static const UChar paraEnds[] = {
        0xd, 0xa, 0x85, 0x2028, 0x2029
    };
    enum {
        iCR, iLF, iNL, iLS, iPS, iCount
    };

    
    if (prev.endsWith(paraEnds + iCR, 1)) {
        if (s.startsWith(paraEnds + iLF, 1)) {
            return 1; 
        } else if (!s.isEmpty()) {
            return 0; 
        } else {
            return -1; 
        }
    }

    const UChar *u = s.getBuffer(), *limit = u + s.length();
    UChar c;

    while (u < limit) {
        c = *u++;
        if (
            ((c < uSP) && (c == uCR || c == uLF)) ||
            (c == uNL) ||
            ((c & uLS) == uLS)
        ) {
            if (c == uCR) {
                
                if (u == limit) {
                    return -1; 
                } else if (*u == uLF) {
                    ++u; 
                }
            }
            return (int32_t)(u - s.getBuffer());
        }
    }

    return -1; 
}

enum {
    CNV_NO_FEFF,    
    CNV_WITH_FEFF,  
    CNV_ADDS_FEFF   
};

static inline UChar
nibbleToHex(uint8_t n) {
    n &= 0xf;
    return
        n <= 9 ?
            (UChar)(0x30 + n) :
            (UChar)((0x61 - 10) + n);
}




static int32_t
cnvSigType(UConverter *cnv) {
    UErrorCode err;
    int32_t result;

    
    USet *set = uset_open(1, 0);
    err = U_ZERO_ERROR;
    ucnv_getUnicodeSet(cnv, set, UCNV_ROUNDTRIP_SET, &err);
    if (U_SUCCESS(err) && uset_contains(set, uSig)) {
        result = CNV_WITH_FEFF;
    } else {
        result = CNV_NO_FEFF; 
    }
    uset_close(set);

    if (result == CNV_WITH_FEFF) {
        
        const UChar a[1] = { 0x61 }; 
        const UChar *in;

        char buffer[20];
        char *out;

        in = a;
        out = buffer;
        err = U_ZERO_ERROR;
        ucnv_fromUnicode(cnv,
            &out, buffer + sizeof(buffer),
            &in, a + 1,
            NULL, TRUE, &err);
        ucnv_resetFromUnicode(cnv);

        if (NULL != ucnv_detectUnicodeSignature(buffer, (int32_t)(out - buffer), NULL, &err) &&
            U_SUCCESS(err)
        ) {
            result = CNV_ADDS_FEFF;
        }
    }

    return result;
}

class ConvertFile {
public:
    ConvertFile() :
        buf(NULL), outbuf(NULL), fromoffsets(NULL),
        bufsz(0), signature(0) {}

    void
    setBufferSize(size_t bufferSize) {
        bufsz = bufferSize;

        buf = new char[2 * bufsz];
        outbuf = buf + bufsz;

        
        fromoffsets = new int32_t[bufsz + 1];
    }

    ~ConvertFile() {
        delete [] buf;
        delete [] fromoffsets;
    }

    UBool convertFile(const char *pname,
                      const char *fromcpage,
                      UConverterToUCallback toucallback,
                      const void *touctxt,
                      const char *tocpage,
                      UConverterFromUCallback fromucallback,
                      const void *fromuctxt,
                      UBool fallback,
                      const char *translit,
                      const char *infilestr,
                      FILE * outfile, int verbose);
private:
    friend int main(int argc, char **argv);

    char *buf, *outbuf;
    int32_t *fromoffsets;

    size_t bufsz;
    int8_t signature; 
};


UBool
ConvertFile::convertFile(const char *pname,
                         const char *fromcpage,
                         UConverterToUCallback toucallback,
                         const void *touctxt,
                         const char *tocpage,
                         UConverterFromUCallback fromucallback,
                         const void *fromuctxt,
                         UBool fallback,
                         const char *translit,
                         const char *infilestr,
                         FILE * outfile, int verbose)
{
    FILE *infile;
    UBool ret = TRUE;
    UConverter *convfrom = 0;
    UConverter *convto = 0;
    UErrorCode err = U_ZERO_ERROR;
    UBool flush;
    UBool closeFile = FALSE;
    const char *cbufp, *prevbufp;
    char *bufp;

    uint32_t infoffset = 0, outfoffset = 0;   

    const UChar *unibuf, *unibufbp;
    UChar *unibufp;

    size_t rd, wr;

#if !UCONFIG_NO_TRANSLITERATION
    Transliterator *t = 0;      
    UnicodeString chunk;        
#endif
    UnicodeString u;            
    int32_t ulen;

    
    
    
    UBool useOffsets = TRUE;

    

    if (infilestr != 0 && strcmp(infilestr, "-")) {
        infile = fopen(infilestr, "rb");
        if (infile == 0) {
            UnicodeString str1(infilestr, "");
            str1.append((UChar32) 0);
            UnicodeString str2(strerror(errno), "");
            str2.append((UChar32) 0);
            initMsg(pname);
            u_wmsg(stderr, "cantOpenInputF", str1.getBuffer(), str2.getBuffer());
            return FALSE;
        }
        closeFile = TRUE;
    } else {
        infilestr = "-";
        infile = stdin;
#ifdef USE_FILENO_BINARY_MODE
        if (setmode(fileno(stdin), O_BINARY) == -1) {
            initMsg(pname);
            u_wmsg(stderr, "cantSetInBinMode");
            return FALSE;
        }
#endif
    }

    if (verbose) {
        fprintf(stderr, "%s:\n", infilestr);
    }

#if !UCONFIG_NO_TRANSLITERATION
    

    if (translit != NULL && *translit) {
        UParseError parse;
        UnicodeString str(translit), pestr;

        

        parse.line = -1;

        if (uprv_strchr(translit, ':') || uprv_strchr(translit, '>') || uprv_strchr(translit, '<') || uprv_strchr(translit, '>')) {
            t = Transliterator::createFromRules("Uconv", str, UTRANS_FORWARD, parse, err);
        } else {
            t = Transliterator::createInstance(translit, UTRANS_FORWARD, err);
        }

        if (U_FAILURE(err)) {
            str.append((UChar32) 0);
            initMsg(pname);

            if (parse.line >= 0) {
                UChar linebuf[20], offsetbuf[20];
                uprv_itou(linebuf, 20, parse.line, 10, 0);
                uprv_itou(offsetbuf, 20, parse.offset, 10, 0);
                u_wmsg(stderr, "cantCreateTranslitParseErr", str.getTerminatedBuffer(),
                    u_wmsg_errorName(err), linebuf, offsetbuf);
            } else {
                u_wmsg(stderr, "cantCreateTranslit", str.getTerminatedBuffer(),
                    u_wmsg_errorName(err));
            }

            if (t) {
                delete t;
                t = 0;
            }
            goto error_exit;
        }

        useOffsets = FALSE;
    }
#endif

    
    
    

    convfrom = ucnv_open(fromcpage, &err);
    if (U_FAILURE(err)) {
        UnicodeString str(fromcpage, "");
        initMsg(pname);
        u_wmsg(stderr, "cantOpenFromCodeset", str.getTerminatedBuffer(),
            u_wmsg_errorName(err));
        goto error_exit;
    }
    ucnv_setToUCallBack(convfrom, toucallback, touctxt, 0, 0, &err);
    if (U_FAILURE(err)) {
        initMsg(pname);
        u_wmsg(stderr, "cantSetCallback", u_wmsg_errorName(err));
        goto error_exit;
    }

    convto = ucnv_open(tocpage, &err);
    if (U_FAILURE(err)) {
        UnicodeString str(tocpage, "");
        initMsg(pname);
        u_wmsg(stderr, "cantOpenToCodeset", str.getTerminatedBuffer(),
            u_wmsg_errorName(err));
        goto error_exit;
    }
    ucnv_setFromUCallBack(convto, fromucallback, fromuctxt, 0, 0, &err);
    if (U_FAILURE(err)) {
        initMsg(pname);
        u_wmsg(stderr, "cantSetCallback", u_wmsg_errorName(err));
        goto error_exit;
    }
    ucnv_setFallback(convto, fallback);

    UBool willexit, fromSawEndOfBytes, toSawEndOfUnicode;
    int8_t sig;

    
    sig = signature;
    rd = 0;

    do {
        willexit = FALSE;

        
        infoffset += rd;

        rd = fread(buf, 1, bufsz, infile);
        if (ferror(infile) != 0) {
            UnicodeString str(strerror(errno));
            initMsg(pname);
            u_wmsg(stderr, "cantRead", str.getTerminatedBuffer());
            goto error_exit;
        }

        
        
        
        
        
        
        
        
        
        

        cbufp = buf;
        flush = (UBool)(rd != bufsz);

        
        do {
            
            prevbufp = cbufp;

            unibuf = unibufp = u.getBuffer((int32_t)bufsz);

            
            
            ucnv_toUnicode(convfrom, &unibufp, unibuf + bufsz, &cbufp,
                buf + rd, useOffsets ? fromoffsets : NULL, flush, &err);

            ulen = (int32_t)(unibufp - unibuf);
            u.releaseBuffer(U_SUCCESS(err) ? ulen : 0);

            
            
            
            
            
            
            
            
            
            fromSawEndOfBytes = (UBool)U_SUCCESS(err);

            if (err == U_BUFFER_OVERFLOW_ERROR) {
                err = U_ZERO_ERROR;
            } else if (U_FAILURE(err)) {
                char pos[32], errorBytes[32];
                int8_t i, length, errorLength;

                UErrorCode localError = U_ZERO_ERROR;
                errorLength = (int8_t)sizeof(errorBytes);
                ucnv_getInvalidChars(convfrom, errorBytes, &errorLength, &localError);
                if (U_FAILURE(localError) || errorLength == 0) {
                    errorLength = 1;
                }

                
                
                
                
                length =
                    (int8_t)sprintf(pos, "%d",
                        (int)(infoffset + (cbufp - buf) - errorLength));

                
                UnicodeString str;
                for (i = 0; i < errorLength; ++i) {
                    if (i > 0) {
                        str.append((UChar)uSP);
                    }
                    str.append(nibbleToHex((uint8_t)errorBytes[i] >> 4));
                    str.append(nibbleToHex((uint8_t)errorBytes[i]));
                }

                initMsg(pname);
                u_wmsg(stderr, "problemCvtToU",
                        UnicodeString(pos, length, "").getTerminatedBuffer(),
                        str.getTerminatedBuffer(),
                        u_wmsg_errorName(err));

                willexit = TRUE;
                err = U_ZERO_ERROR; 
            }

            
            

            if (ulen == 0) {
                continue;
            }

            
            if (sig < 0) {
                if (u.charAt(0) == uSig) {
                    u.remove(0, 1);

                    
                    --ulen;

                    if (useOffsets) {
                        
                        
                        memmove(fromoffsets, fromoffsets + 1, ulen * 4);
                    }

                }
                sig = 0;
            }

#if !UCONFIG_NO_TRANSLITERATION
            

            
            
            
            
            
            
            
            if (t != NULL) {
                UnicodeString out;
                int32_t chunkLimit;

                do {
                    chunkLimit = getChunkLimit(chunk, u);
                    if (chunkLimit < 0 && flush && fromSawEndOfBytes) {
                        
                        chunkLimit = u.length();
                    }
                    if (chunkLimit >= 0) {
                        
                        chunk.append(u, 0, chunkLimit);
                        u.remove(0, chunkLimit);
                        t->transliterate(chunk);

                        
                        out.append(chunk);
                        chunk.remove();
                    } else {
                        
                        chunk.append(u);
                        break;
                    }
                } while (!u.isEmpty());

                u = out;
                ulen = u.length();
            }
#endif

            
            
            if (sig > 0) {
                if (u.charAt(0) != uSig && cnvSigType(convto) == CNV_WITH_FEFF) {
                    u.insert(0, (UChar)uSig);

                    if (useOffsets) {
                        
                        
                        memmove(fromoffsets + 1, fromoffsets, ulen * 4);
                        fromoffsets[0] = -1;
                    }

                    
                    ++ulen;
                }
                sig = 0;
            }

            
            
            
            
            

            unibuf = unibufbp = u.getBuffer();

            do {
                bufp = outbuf;

                
                
                
                ucnv_fromUnicode(convto, &bufp, outbuf + bufsz,
                                 &unibufbp,
                                 unibuf + ulen,
                                 NULL, (UBool)(flush && fromSawEndOfBytes), &err);

                
                
                
                toSawEndOfUnicode = (UBool)U_SUCCESS(err);

                if (err == U_BUFFER_OVERFLOW_ERROR) {
                    err = U_ZERO_ERROR;
                } else if (U_FAILURE(err)) {
                    UChar errorUChars[4];
                    const char *errtag;
                    char pos[32];
                    UChar32 c;
                    int8_t i, length, errorLength;

                    UErrorCode localError = U_ZERO_ERROR;
                    errorLength = (int8_t)LENGTHOF(errorUChars);
                    ucnv_getInvalidUChars(convto, errorUChars, &errorLength, &localError);
                    if (U_FAILURE(localError) || errorLength == 0) {
                        
                        errorLength = 1;
                    }

                    int32_t ferroffset;

                    if (useOffsets) {
                        
                        ferroffset = (int32_t)((unibufbp - unibuf) - errorLength);
                        if (ferroffset < 0) {
                            
                            ferroffset = 0;
                        }

                        
                        
                        int32_t fromoffset;
                        do {
                            fromoffset = fromoffsets[ferroffset];
                        } while (fromoffset < 0 && --ferroffset >= 0);

                        
                        
                        
                        
                        ferroffset = infoffset + (prevbufp - buf) + fromoffset;
                        errtag = "problemCvtFromU";
                    } else {
                        
                        

                        
                        ferroffset = (int32_t)(outfoffset + (bufp - outbuf));
                        errtag = "problemCvtFromUOut";
                    }

                    length = (int8_t)sprintf(pos, "%u", (int)ferroffset);

                    
                    UnicodeString str;
                    for (i = 0; i < errorLength;) {
                        if (i > 0) {
                            str.append((UChar)uSP);
                        }
                        U16_NEXT(errorUChars, i, errorLength, c);
                        if (c >= 0x100000) {
                            str.append(nibbleToHex((uint8_t)(c >> 20)));
                        }
                        if (c >= 0x10000) {
                            str.append(nibbleToHex((uint8_t)(c >> 16)));
                        }
                        str.append(nibbleToHex((uint8_t)(c >> 12)));
                        str.append(nibbleToHex((uint8_t)(c >> 8)));
                        str.append(nibbleToHex((uint8_t)(c >> 4)));
                        str.append(nibbleToHex((uint8_t)c));
                    }

                    initMsg(pname);
                    u_wmsg(stderr, errtag,
                            UnicodeString(pos, length, "").getTerminatedBuffer(),
                            str.getTerminatedBuffer(),
                           u_wmsg_errorName(err));
                    u_wmsg(stderr, "errorUnicode", str.getTerminatedBuffer());

                    willexit = TRUE;
                    err = U_ZERO_ERROR; 
                }

                
                

                
                size_t outlen = (size_t) (bufp - outbuf);
                outfoffset += (int32_t)(wr = fwrite(outbuf, 1, outlen, outfile));
                if (wr != outlen) {
                    UnicodeString str(strerror(errno));
                    initMsg(pname);
                    u_wmsg(stderr, "cantWrite", str.getTerminatedBuffer());
                    willexit = TRUE;
                }

                if (willexit) {
                    goto error_exit;
                }
            } while (!toSawEndOfUnicode);
        } while (!fromSawEndOfBytes);
    } while (!flush);           
                                
                                

    goto normal_exit;

error_exit:
    ret = FALSE;

normal_exit:
    

    ucnv_close(convfrom);
    ucnv_close(convto);

#if !UCONFIG_NO_TRANSLITERATION
    delete t;
#endif

    if (closeFile) {
        fclose(infile);
    }

    return ret;
}

static void usage(const char *pname, int ecode) {
    const UChar *msg;
    int32_t msgLen;
    UErrorCode err = U_ZERO_ERROR;
    FILE *fp = ecode ? stderr : stdout;
    int res;

    initMsg(pname);
    msg =
        ures_getStringByKey(gBundle, ecode ? "lcUsageWord" : "ucUsageWord",
                            &msgLen, &err);
    UnicodeString upname(pname, (int32_t)(uprv_strlen(pname) + 1));
    UnicodeString mname(msg, msgLen + 1);

    res = u_wmsg(fp, "usage", mname.getBuffer(), upname.getBuffer());
    if (!ecode) {
        if (!res) {
            fputc('\n', fp);
        }
        if (!u_wmsg(fp, "help")) {
            

            int i, count =
                sizeof(transcode_callbacks) / sizeof(*transcode_callbacks);
            for (i = 0; i < count; ++i) {
                fprintf(fp, " %s", transcode_callbacks[i].name);
            }
            fputc('\n', fp);
        }
    }

    exit(ecode);
}

extern int
main(int argc, char **argv)
{
    FILE *outfile;
    int ret = 0;

    size_t bufsz = DEFAULT_BUFSZ;

    const char *fromcpage = 0;
    const char *tocpage = 0;
    const char *translit = 0;
    const char *outfilestr = 0;
    UBool fallback = FALSE;

    UConverterFromUCallback fromucallback = UCNV_FROM_U_CALLBACK_STOP;
    const void *fromuctxt = 0;
    UConverterToUCallback toucallback = UCNV_TO_U_CALLBACK_STOP;
    const void *touctxt = 0;

    char **iter, **remainArgv, **remainArgvLimit;
    char **end = argv + argc;

    const char *pname;

    UBool printConvs = FALSE, printCanon = FALSE, printTranslits = FALSE;
    const char *printName = 0;

    UBool verbose = FALSE;
    UErrorCode status = U_ZERO_ERROR;

    ConvertFile cf;

    
    u_init(&status);
    if (U_FAILURE(status)) {
        fprintf(stderr, "%s: can not initialize ICU.  status = %s\n",
            argv[0], u_errorName(status));
        exit(1);
    }

    
    pname = uprv_strrchr(*argv, U_FILE_SEP_CHAR);
#if U_PLATFORM_USES_ONLY_WIN32_API
    if (!pname) {
        pname = uprv_strrchr(*argv, '/');
    }
#endif
    if (!pname) {
        pname = *argv;
    } else {
        ++pname;
    }

    
    

    remainArgv = remainArgvLimit = argv + 1;
    for (iter = argv + 1; iter != end; iter++) {
        
        if (strcmp("-f", *iter) == 0 || !strcmp("--from-code", *iter)) {
            iter++;
            if (iter != end)
                fromcpage = *iter;
            else
                usage(pname, 1);
        } else if (strcmp("-t", *iter) == 0 || !strcmp("--to-code", *iter)) {
            iter++;
            if (iter != end)
                tocpage = *iter;
            else
                usage(pname, 1);
        } else if (strcmp("-x", *iter) == 0) {
            iter++;
            if (iter != end)
                translit = *iter;
            else
                usage(pname, 1);
        } else if (!strcmp("--fallback", *iter)) {
            fallback = TRUE;
        } else if (!strcmp("--no-fallback", *iter)) {
            fallback = FALSE;
        } else if (strcmp("-b", *iter) == 0 || !strcmp("--block-size", *iter)) {
            iter++;
            if (iter != end) {
                bufsz = atoi(*iter);
                if ((int) bufsz <= 0) {
                    initMsg(pname);
                    UnicodeString str(*iter);
                    initMsg(pname);
                    u_wmsg(stderr, "badBlockSize", str.getTerminatedBuffer());
                    return 3;
                }
            } else {
                usage(pname, 1);
            }
        } else if (strcmp("-l", *iter) == 0 || !strcmp("--list", *iter)) {
            if (printTranslits) {
                usage(pname, 1);
            }
            printConvs = TRUE;
        } else if (strcmp("--default-code", *iter) == 0) {
            if (printTranslits) {
                usage(pname, 1);
            }
            printName = ucnv_getDefaultName();
        } else if (strcmp("--list-code", *iter) == 0) {
            if (printTranslits) {
                usage(pname, 1);
            }

            iter++;
            if (iter != end) {
                UErrorCode e = U_ZERO_ERROR;
                printName = ucnv_getAlias(*iter, 0, &e);
                if (U_FAILURE(e) || !printName) {
                    UnicodeString str(*iter);
                    initMsg(pname);
                    u_wmsg(stderr, "noSuchCodeset", str.getTerminatedBuffer());
                    return 2;
                }
            } else
                usage(pname, 1);
        } else if (strcmp("--canon", *iter) == 0) {
            printCanon = TRUE;
        } else if (strcmp("-L", *iter) == 0
            || !strcmp("--list-transliterators", *iter)) {
            if (printConvs) {
                usage(pname, 1);
            }
            printTranslits = TRUE;
        } else if (strcmp("-h", *iter) == 0 || !strcmp("-?", *iter)
            || !strcmp("--help", *iter)) {
            usage(pname, 0);
        } else if (!strcmp("-c", *iter)) {
            fromucallback = UCNV_FROM_U_CALLBACK_SKIP;
        } else if (!strcmp("--to-callback", *iter)) {
            iter++;
            if (iter != end) {
                const struct callback_ent *cbe = findCallback(*iter);
                if (cbe) {
                    fromucallback = cbe->fromu;
                    fromuctxt = cbe->fromuctxt;
                } else {
                    UnicodeString str(*iter);
                    initMsg(pname);
                    u_wmsg(stderr, "unknownCallback", str.getTerminatedBuffer());
                    return 4;
                }
            } else {
                usage(pname, 1);
            }
        } else if (!strcmp("--from-callback", *iter)) {
            iter++;
            if (iter != end) {
                const struct callback_ent *cbe = findCallback(*iter);
                if (cbe) {
                    toucallback = cbe->tou;
                    touctxt = cbe->touctxt;
                } else {
                    UnicodeString str(*iter);
                    initMsg(pname);
                    u_wmsg(stderr, "unknownCallback", str.getTerminatedBuffer());
                    return 4;
                }
            } else {
                usage(pname, 1);
            }
        } else if (!strcmp("-i", *iter)) {
            toucallback = UCNV_TO_U_CALLBACK_SKIP;
        } else if (!strcmp("--callback", *iter)) {
            iter++;
            if (iter != end) {
                const struct callback_ent *cbe = findCallback(*iter);
                if (cbe) {
                    fromucallback = cbe->fromu;
                    fromuctxt = cbe->fromuctxt;
                    toucallback = cbe->tou;
                    touctxt = cbe->touctxt;
                } else {
                    UnicodeString str(*iter);
                    initMsg(pname);
                    u_wmsg(stderr, "unknownCallback", str.getTerminatedBuffer());
                    return 4;
                }
            } else {
                usage(pname, 1);
            }
        } else if (!strcmp("-s", *iter) || !strcmp("--silent", *iter)) {
            verbose = FALSE;
        } else if (!strcmp("-v", *iter) || !strcmp("--verbose", *iter)) {
            verbose = TRUE;
        } else if (!strcmp("-V", *iter) || !strcmp("--version", *iter)) {
            printf("%s v2.1  ICU " U_ICU_VERSION "\n", pname);
            return 0;
        } else if (!strcmp("-o", *iter) || !strcmp("--output", *iter)) {
            ++iter;
            if (iter != end && !outfilestr) {
                outfilestr = *iter;
            } else {
                usage(pname, 1);
            }
        } else if (0 == strcmp("--add-signature", *iter)) {
            cf.signature = 1;
        } else if (0 == strcmp("--remove-signature", *iter)) {
            cf.signature = -1;
        } else if (**iter == '-' && (*iter)[1]) {
            usage(pname, 1);
        } else {
            
            *remainArgvLimit++ = *iter;
        }
    }

    if (printConvs || printName) {
        return printConverters(pname, printName, printCanon) ? 2 : 0;
    } else if (printTranslits) {
        return printTransliterators(printCanon) ? 3 : 0;
    }

    if (!fromcpage || !uprv_strcmp(fromcpage, "-")) {
        fromcpage = ucnv_getDefaultName();
    }
    if (!tocpage || !uprv_strcmp(tocpage, "-")) {
        tocpage = ucnv_getDefaultName();
    }

    
    if (outfilestr != 0 && strcmp(outfilestr, "-")) {
        outfile = fopen(outfilestr, "wb");
        if (outfile == 0) {
            UnicodeString str1(outfilestr, "");
            UnicodeString str2(strerror(errno), "");
            initMsg(pname);
            u_wmsg(stderr, "cantCreateOutputF",
                str1.getBuffer(), str2.getBuffer());
            return 1;
        }
    } else {
        outfilestr = "-";
        outfile = stdout;
#ifdef USE_FILENO_BINARY_MODE
        if (setmode(fileno(outfile), O_BINARY) == -1) {
            u_wmsg(stderr, "cantSetOutBinMode");
            exit(-1);
        }
#endif
    }

    


    cf.setBufferSize(bufsz);

    if(remainArgv < remainArgvLimit) {
        for (iter = remainArgv; iter != remainArgvLimit; iter++) {
            if (!cf.convertFile(
                    pname, fromcpage, toucallback, touctxt, tocpage,
                    fromucallback, fromuctxt, fallback, translit, *iter,
                    outfile, verbose)
            ) {
                goto error_exit;
            }
        }
    } else {
        if (!cf.convertFile(
                pname, fromcpage, toucallback, touctxt, tocpage,
                fromucallback, fromuctxt, fallback, translit, 0,
                outfile, verbose)
        ) {
            goto error_exit;
        }
    }

    goto normal_exit;
error_exit:
#if !UCONFIG_NO_LEGACY_CONVERSION
    ret = 1;
#else 
    fprintf(stderr, "uconv error: UCONFIG_NO_LEGACY_CONVERSION is on. See uconfig.h\n");
#endif
normal_exit:

    if (outfile != stdout) {
        fclose(outfile);
    }

    u_cleanup();

    return ret;
}










