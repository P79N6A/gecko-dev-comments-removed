













#include "unicode/utypes.h"

#if !UCONFIG_NO_IDNA

#include "unicode/idna.h"
#include "unicode/normalizer2.h"
#include "unicode/uscript.h"
#include "unicode/ustring.h"
#include "unicode/utf16.h"
#include "cmemory.h"
#include "cstring.h"
#include "punycode.h"
#include "ubidi_props.h"
#include "ustr_imp.h"

#define LENGTHOF(array) (int32_t)(sizeof(array)/sizeof((array)[0]))









U_NAMESPACE_BEGIN


const uint32_t severeErrors=
    UIDNA_ERROR_LEADING_COMBINING_MARK|
    UIDNA_ERROR_DISALLOWED|
    UIDNA_ERROR_PUNYCODE|
    UIDNA_ERROR_LABEL_HAS_DOT|
    UIDNA_ERROR_INVALID_ACE_LABEL;

static inline UBool
isASCIIString(const UnicodeString &dest) {
    const UChar *s=dest.getBuffer();
    const UChar *limit=s+dest.length();
    while(s<limit) {
        if(*s++>0x7f) {
            return FALSE;
        }
    }
    return TRUE;
}

static UBool
isASCIIOkBiDi(const UChar *s, int32_t length);

static UBool
isASCIIOkBiDi(const char *s, int32_t length);



IDNA::~IDNA() {}

void
IDNA::labelToASCII_UTF8(const StringPiece &label, ByteSink &dest,
                        IDNAInfo &info, UErrorCode &errorCode) const {
    if(U_SUCCESS(errorCode)) {
        UnicodeString destString;
        labelToASCII(UnicodeString::fromUTF8(label), destString,
                     info, errorCode).toUTF8(dest);
    }
}

void
IDNA::labelToUnicodeUTF8(const StringPiece &label, ByteSink &dest,
                         IDNAInfo &info, UErrorCode &errorCode) const {
    if(U_SUCCESS(errorCode)) {
        UnicodeString destString;
        labelToUnicode(UnicodeString::fromUTF8(label), destString,
                       info, errorCode).toUTF8(dest);
    }
}

void
IDNA::nameToASCII_UTF8(const StringPiece &name, ByteSink &dest,
                       IDNAInfo &info, UErrorCode &errorCode) const {
    if(U_SUCCESS(errorCode)) {
        UnicodeString destString;
        nameToASCII(UnicodeString::fromUTF8(name), destString,
                    info, errorCode).toUTF8(dest);
    }
}

void
IDNA::nameToUnicodeUTF8(const StringPiece &name, ByteSink &dest,
                        IDNAInfo &info, UErrorCode &errorCode) const {
    if(U_SUCCESS(errorCode)) {
        UnicodeString destString;
        nameToUnicode(UnicodeString::fromUTF8(name), destString,
                      info, errorCode).toUTF8(dest);
    }
}

UOBJECT_DEFINE_NO_RTTI_IMPLEMENTATION(IDNA)



class UTS46 : public IDNA {
public:
    UTS46(uint32_t options, UErrorCode &errorCode);
    virtual ~UTS46();

    virtual UnicodeString &
    labelToASCII(const UnicodeString &label, UnicodeString &dest,
                 IDNAInfo &info, UErrorCode &errorCode) const;

    virtual UnicodeString &
    labelToUnicode(const UnicodeString &label, UnicodeString &dest,
                   IDNAInfo &info, UErrorCode &errorCode) const;

    virtual UnicodeString &
    nameToASCII(const UnicodeString &name, UnicodeString &dest,
                IDNAInfo &info, UErrorCode &errorCode) const;

    virtual UnicodeString &
    nameToUnicode(const UnicodeString &name, UnicodeString &dest,
                  IDNAInfo &info, UErrorCode &errorCode) const;

    virtual void
    labelToASCII_UTF8(const StringPiece &label, ByteSink &dest,
                      IDNAInfo &info, UErrorCode &errorCode) const;

    virtual void
    labelToUnicodeUTF8(const StringPiece &label, ByteSink &dest,
                       IDNAInfo &info, UErrorCode &errorCode) const;

    virtual void
    nameToASCII_UTF8(const StringPiece &name, ByteSink &dest,
                     IDNAInfo &info, UErrorCode &errorCode) const;

    virtual void
    nameToUnicodeUTF8(const StringPiece &name, ByteSink &dest,
                      IDNAInfo &info, UErrorCode &errorCode) const;

private:
    UnicodeString &
    process(const UnicodeString &src,
            UBool isLabel, UBool toASCII,
            UnicodeString &dest,
            IDNAInfo &info, UErrorCode &errorCode) const;

    void
    processUTF8(const StringPiece &src,
                UBool isLabel, UBool toASCII,
                ByteSink &dest,
                IDNAInfo &info, UErrorCode &errorCode) const;

    UnicodeString &
    processUnicode(const UnicodeString &src,
                   int32_t labelStart, int32_t mappingStart,
                   UBool isLabel, UBool toASCII,
                   UnicodeString &dest,
                   IDNAInfo &info, UErrorCode &errorCode) const;

    
    int32_t
    mapDevChars(UnicodeString &dest, int32_t labelStart, int32_t mappingStart,
                UErrorCode &errorCode) const;

    
    int32_t
    processLabel(UnicodeString &dest,
                 int32_t labelStart, int32_t labelLength,
                 UBool toASCII,
                 IDNAInfo &info, UErrorCode &errorCode) const;
    int32_t
    markBadACELabel(UnicodeString &dest,
                    int32_t labelStart, int32_t labelLength,
                    UBool toASCII, IDNAInfo &info) const;

    void
    checkLabelBiDi(const UChar *label, int32_t labelLength, IDNAInfo &info) const;

    UBool
    isLabelOkContextJ(const UChar *label, int32_t labelLength) const;

    void
    checkLabelContextO(const UChar *label, int32_t labelLength, IDNAInfo &info) const;

    const Normalizer2 &uts46Norm2;  
    uint32_t options;
};

IDNA *
IDNA::createUTS46Instance(uint32_t options, UErrorCode &errorCode) {
    if(U_SUCCESS(errorCode)) {
        IDNA *idna=new UTS46(options, errorCode);
        if(idna==NULL) {
            errorCode=U_MEMORY_ALLOCATION_ERROR;
        } else if(U_FAILURE(errorCode)) {
            delete idna;
            idna=NULL;
        }
        return idna;
    } else {
        return NULL;
    }
}



UTS46::UTS46(uint32_t opt, UErrorCode &errorCode)
        : uts46Norm2(*Normalizer2::getInstance(NULL, "uts46", UNORM2_COMPOSE, errorCode)),
          options(opt) {}

UTS46::~UTS46() {}

UnicodeString &
UTS46::labelToASCII(const UnicodeString &label, UnicodeString &dest,
                    IDNAInfo &info, UErrorCode &errorCode) const {
    return process(label, TRUE, TRUE, dest, info, errorCode);
}

UnicodeString &
UTS46::labelToUnicode(const UnicodeString &label, UnicodeString &dest,
                      IDNAInfo &info, UErrorCode &errorCode) const {
    return process(label, TRUE, FALSE, dest, info, errorCode);
}

UnicodeString &
UTS46::nameToASCII(const UnicodeString &name, UnicodeString &dest,
                   IDNAInfo &info, UErrorCode &errorCode) const {
    process(name, FALSE, TRUE, dest, info, errorCode);
    if( dest.length()>=254 && (info.errors&UIDNA_ERROR_DOMAIN_NAME_TOO_LONG)==0 &&
        isASCIIString(dest) &&
        (dest.length()>254 || dest[253]!=0x2e)
    ) {
        info.errors|=UIDNA_ERROR_DOMAIN_NAME_TOO_LONG;
    }
    return dest;
}

UnicodeString &
UTS46::nameToUnicode(const UnicodeString &name, UnicodeString &dest,
                     IDNAInfo &info, UErrorCode &errorCode) const {
    return process(name, FALSE, FALSE, dest, info, errorCode);
}

void
UTS46::labelToASCII_UTF8(const StringPiece &label, ByteSink &dest,
                         IDNAInfo &info, UErrorCode &errorCode) const {
    processUTF8(label, TRUE, TRUE, dest, info, errorCode);
}

void
UTS46::labelToUnicodeUTF8(const StringPiece &label, ByteSink &dest,
                          IDNAInfo &info, UErrorCode &errorCode) const {
    processUTF8(label, TRUE, FALSE, dest, info, errorCode);
}

void
UTS46::nameToASCII_UTF8(const StringPiece &name, ByteSink &dest,
                        IDNAInfo &info, UErrorCode &errorCode) const {
    processUTF8(name, FALSE, TRUE, dest, info, errorCode);
}

void
UTS46::nameToUnicodeUTF8(const StringPiece &name, ByteSink &dest,
                         IDNAInfo &info, UErrorCode &errorCode) const {
    processUTF8(name, FALSE, FALSE, dest, info, errorCode);
}








static const int8_t asciiData[128]={
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  0,  0, -1,
    
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0, -1, -1, -1, -1, -1, -1,
    
    -1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
     1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1, -1, -1, -1, -1, -1,
    
    -1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, -1, -1, -1, -1, -1
};

UnicodeString &
UTS46::process(const UnicodeString &src,
               UBool isLabel, UBool toASCII,
               UnicodeString &dest,
               IDNAInfo &info, UErrorCode &errorCode) const {
    
    
    
    if(U_FAILURE(errorCode)) {
        dest.setToBogus();
        return dest;
    }
    const UChar *srcArray=src.getBuffer();
    if(&dest==&src || srcArray==NULL) {
        errorCode=U_ILLEGAL_ARGUMENT_ERROR;
        dest.setToBogus();
        return dest;
    }
    
    dest.remove();
    info.reset();
    int32_t srcLength=src.length();
    if(srcLength==0) {
        if(toASCII) {
            info.errors|=UIDNA_ERROR_EMPTY_LABEL;
        }
        return dest;
    }
    UChar *destArray=dest.getBuffer(srcLength);
    if(destArray==NULL) {
        errorCode=U_MEMORY_ALLOCATION_ERROR;
        return dest;
    }
    
    UBool disallowNonLDHDot=(options&UIDNA_USE_STD3_RULES)!=0;
    int32_t labelStart=0;
    int32_t i;
    for(i=0;; ++i) {
        if(i==srcLength) {
            if(toASCII) {
                if((i-labelStart)>63) {
                    info.labelErrors|=UIDNA_ERROR_LABEL_TOO_LONG;
                }
                
                if(!isLabel && i>=254 && (i>254 || labelStart<i)) {
                    info.errors|=UIDNA_ERROR_DOMAIN_NAME_TOO_LONG;
                }
            }
            info.errors|=info.labelErrors;
            dest.releaseBuffer(i);
            return dest;
        }
        UChar c=srcArray[i];
        if(c>0x7f) {
            break;
        }
        int cData=asciiData[c];
        if(cData>0) {
            destArray[i]=c+0x20;  
        } else if(cData<0 && disallowNonLDHDot) {
            break;  
        } else {
            destArray[i]=c;
            if(c==0x2d) {  
                if(i==(labelStart+3) && srcArray[i-1]==0x2d) {
                    
                    ++i;  
                    break;
                }
                if(i==labelStart) {
                    
                    info.labelErrors|=UIDNA_ERROR_LEADING_HYPHEN;
                }
                if((i+1)==srcLength || srcArray[i+1]==0x2e) {
                    
                    info.labelErrors|=UIDNA_ERROR_TRAILING_HYPHEN;
                }
            } else if(c==0x2e) {  
                if(isLabel) {
                    
                    ++i;  
                    break;
                }
                if(toASCII) {
                    
                    if(i==labelStart && i<(srcLength-1)) {
                        info.labelErrors|=UIDNA_ERROR_EMPTY_LABEL;
                    } else if((i-labelStart)>63) {
                        info.labelErrors|=UIDNA_ERROR_LABEL_TOO_LONG;
                    }
                }
                info.errors|=info.labelErrors;
                info.labelErrors=0;
                labelStart=i+1;
            }
        }
    }
    info.errors|=info.labelErrors;
    dest.releaseBuffer(i);
    processUnicode(src, labelStart, i, isLabel, toASCII, dest, info, errorCode);
    if( info.isBiDi && U_SUCCESS(errorCode) && (info.errors&severeErrors)==0 &&
        (!info.isOkBiDi || (labelStart>0 && !isASCIIOkBiDi(dest.getBuffer(), labelStart)))
    ) {
        info.errors|=UIDNA_ERROR_BIDI;
    }
    return dest;
}

void
UTS46::processUTF8(const StringPiece &src,
                   UBool isLabel, UBool toASCII,
                   ByteSink &dest,
                   IDNAInfo &info, UErrorCode &errorCode) const {
    if(U_FAILURE(errorCode)) {
        return;
    }
    const char *srcArray=src.data();
    int32_t srcLength=src.length();
    if(srcArray==NULL && srcLength!=0) {
        errorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return;
    }
    
    info.reset();
    if(srcLength==0) {
        if(toASCII) {
            info.errors|=UIDNA_ERROR_EMPTY_LABEL;
        }
        dest.Flush();
        return;
    }
    UnicodeString destString;
    int32_t labelStart=0;
    if(srcLength<=256) {  
        
        char stackArray[256];
        int32_t destCapacity;
        char *destArray=dest.GetAppendBuffer(srcLength, srcLength+20,
                                             stackArray, LENGTHOF(stackArray), &destCapacity);
        UBool disallowNonLDHDot=(options&UIDNA_USE_STD3_RULES)!=0;
        int32_t i;
        for(i=0;; ++i) {
            if(i==srcLength) {
                if(toASCII) {
                    if((i-labelStart)>63) {
                        info.labelErrors|=UIDNA_ERROR_LABEL_TOO_LONG;
                    }
                    
                    if(!isLabel && i>=254 && (i>254 || labelStart<i)) {
                        info.errors|=UIDNA_ERROR_DOMAIN_NAME_TOO_LONG;
                    }
                }
                info.errors|=info.labelErrors;
                dest.Append(destArray, i);
                dest.Flush();
                return;
            }
            char c=srcArray[i];
            if((int8_t)c<0) {  
                break;
            }
            int cData=asciiData[(int)c];  
            if(cData>0) {
                destArray[i]=c+0x20;  
            } else if(cData<0 && disallowNonLDHDot) {
                break;  
            } else {
                destArray[i]=c;
                if(c==0x2d) {  
                    if(i==(labelStart+3) && srcArray[i-1]==0x2d) {
                        
                        break;
                    }
                    if(i==labelStart) {
                        
                        info.labelErrors|=UIDNA_ERROR_LEADING_HYPHEN;
                    }
                    if((i+1)==srcLength || srcArray[i+1]==0x2e) {
                        
                        info.labelErrors|=UIDNA_ERROR_TRAILING_HYPHEN;
                    }
                } else if(c==0x2e) {  
                    if(isLabel) {
                        break;  
                    }
                    if(toASCII) {
                        
                        if(i==labelStart && i<(srcLength-1)) {
                            info.labelErrors|=UIDNA_ERROR_EMPTY_LABEL;
                        } else if((i-labelStart)>63) {
                            info.labelErrors|=UIDNA_ERROR_LABEL_TOO_LONG;
                        }
                    }
                    info.errors|=info.labelErrors;
                    info.labelErrors=0;
                    labelStart=i+1;
                }
            }
        }
        info.errors|=info.labelErrors;
        
        int32_t mappingStart=i-labelStart;
        destString=UnicodeString::fromUTF8(StringPiece(destArray+labelStart, mappingStart));
        
        dest.Append(destArray, labelStart);
        processUnicode(UnicodeString::fromUTF8(StringPiece(src, labelStart)), 0, mappingStart,
                       isLabel, toASCII,
                       destString, info, errorCode);
    } else {
        
        processUnicode(UnicodeString::fromUTF8(src), 0, 0,
                       isLabel, toASCII,
                       destString, info, errorCode);
    }
    destString.toUTF8(dest);  
    if(toASCII && !isLabel) {
        
        
        int32_t length=labelStart+destString.length();
        if( length>=254 && isASCIIString(destString) &&
            (length>254 ||
             (labelStart<254 && destString[253-labelStart]!=0x2e))
        ) {
            info.errors|=UIDNA_ERROR_DOMAIN_NAME_TOO_LONG;
        }
    }
    if( info.isBiDi && U_SUCCESS(errorCode) && (info.errors&severeErrors)==0 &&
        (!info.isOkBiDi || (labelStart>0 && !isASCIIOkBiDi(srcArray, labelStart)))
    ) {
        info.errors|=UIDNA_ERROR_BIDI;
    }
}

UnicodeString &
UTS46::processUnicode(const UnicodeString &src,
                      int32_t labelStart, int32_t mappingStart,
                      UBool isLabel, UBool toASCII,
                      UnicodeString &dest,
                      IDNAInfo &info, UErrorCode &errorCode) const {
    if(mappingStart==0) {
        uts46Norm2.normalize(src, dest, errorCode);
    } else {
        uts46Norm2.normalizeSecondAndAppend(dest, src.tempSubString(mappingStart), errorCode);
    }
    if(U_FAILURE(errorCode)) {
        return dest;
    }
    UBool doMapDevChars=
        toASCII ? (options&UIDNA_NONTRANSITIONAL_TO_ASCII)==0 :
                  (options&UIDNA_NONTRANSITIONAL_TO_UNICODE)==0;
    const UChar *destArray=dest.getBuffer();
    int32_t destLength=dest.length();
    int32_t labelLimit=labelStart;
    while(labelLimit<destLength) {
        UChar c=destArray[labelLimit];
        if(c==0x2e && !isLabel) {
            int32_t labelLength=labelLimit-labelStart;
            int32_t newLength=processLabel(dest, labelStart, labelLength,
                                            toASCII, info, errorCode);
            info.errors|=info.labelErrors;
            info.labelErrors=0;
            if(U_FAILURE(errorCode)) {
                return dest;
            }
            destArray=dest.getBuffer();
            destLength+=newLength-labelLength;
            labelLimit=labelStart+=newLength+1;
        } else if(0xdf<=c && c<=0x200d && (c==0xdf || c==0x3c2 || c>=0x200c)) {
            info.isTransDiff=TRUE;
            if(doMapDevChars) {
                destLength=mapDevChars(dest, labelStart, labelLimit, errorCode);
                if(U_FAILURE(errorCode)) {
                    return dest;
                }
                destArray=dest.getBuffer();
                
                
                doMapDevChars=FALSE;
            } else {
                ++labelLimit;
            }
        } else {
            ++labelLimit;
        }
    }
    
    
    
    if(0==labelStart || labelStart<labelLimit) {
        processLabel(dest, labelStart, labelLimit-labelStart,
                      toASCII, info, errorCode);
        info.errors|=info.labelErrors;
    }
    return dest;
}

int32_t
UTS46::mapDevChars(UnicodeString &dest, int32_t labelStart, int32_t mappingStart,
                   UErrorCode &errorCode) const {
    int32_t length=dest.length();
    UChar *s=dest.getBuffer(dest[mappingStart]==0xdf ? length+1 : length);
    if(s==NULL) {
        errorCode=U_MEMORY_ALLOCATION_ERROR;
        return length;
    }
    int32_t capacity=dest.getCapacity();
    UBool didMapDevChars=FALSE;
    int32_t readIndex=mappingStart, writeIndex=mappingStart;
    do {
        UChar c=s[readIndex++];
        switch(c) {
        case 0xdf:
            
            didMapDevChars=TRUE;
            s[writeIndex++]=0x73;  
            
            if(writeIndex==readIndex) {
                if(length==capacity) {
                    dest.releaseBuffer(length);
                    s=dest.getBuffer(length+1);
                    if(s==NULL) {
                        errorCode=U_MEMORY_ALLOCATION_ERROR;
                        return length;
                    }
                    capacity=dest.getCapacity();
                }
                u_memmove(s+writeIndex+1, s+writeIndex, length-writeIndex);
                ++readIndex;
            }
            s[writeIndex++]=0x73;
            ++length;
            break;
        case 0x3c2:  
            didMapDevChars=TRUE;
            s[writeIndex++]=0x3c3;
            break;
        case 0x200c:  
        case 0x200d:  
            didMapDevChars=TRUE;
            --length;
            break;
        default:
            
            s[writeIndex++]=c;
            break;
        }
    } while(writeIndex<length);
    dest.releaseBuffer(length);
    if(didMapDevChars) {
        
        
        
        UnicodeString normalized;
        uts46Norm2.normalize(dest.tempSubString(labelStart), normalized, errorCode);
        if(U_SUCCESS(errorCode)) {
            dest.replace(labelStart, 0x7fffffff, normalized);
            return dest.length();
        }
    }
    return length;
}




static inline UBool
isNonASCIIDisallowedSTD3Valid(UChar32 c) {
    return c==0x2260 || c==0x226E || c==0x226F;
}






static int32_t
replaceLabel(UnicodeString &dest, int32_t destLabelStart, int32_t destLabelLength,
             const UnicodeString &label, int32_t labelLength) {
    if(&label!=&dest) {
        dest.replace(destLabelStart, destLabelLength, label);
    }
    return labelLength;
}

int32_t
UTS46::processLabel(UnicodeString &dest,
                    int32_t labelStart, int32_t labelLength,
                    UBool toASCII,
                    IDNAInfo &info, UErrorCode &errorCode) const {
    UnicodeString fromPunycode;
    UnicodeString *labelString;
    const UChar *label=dest.getBuffer()+labelStart;
    int32_t destLabelStart=labelStart;
    int32_t destLabelLength=labelLength;
    UBool wasPunycode;
    if(labelLength>=4 && label[0]==0x78 && label[1]==0x6e && label[2]==0x2d && label[3]==0x2d) {
        
        wasPunycode=TRUE;
        UChar *unicodeBuffer=fromPunycode.getBuffer(-1);  
        if(unicodeBuffer==NULL) {
            
            errorCode=U_MEMORY_ALLOCATION_ERROR;
            return labelLength;
        }
        UErrorCode punycodeErrorCode=U_ZERO_ERROR;
        int32_t unicodeLength=u_strFromPunycode(label+4, labelLength-4,
                                                unicodeBuffer, fromPunycode.getCapacity(),
                                                NULL, &punycodeErrorCode);
        if(punycodeErrorCode==U_BUFFER_OVERFLOW_ERROR) {
            fromPunycode.releaseBuffer(0);
            unicodeBuffer=fromPunycode.getBuffer(unicodeLength);
            if(unicodeBuffer==NULL) {
                errorCode=U_MEMORY_ALLOCATION_ERROR;
                return labelLength;
            }
            punycodeErrorCode=U_ZERO_ERROR;
            unicodeLength=u_strFromPunycode(label+4, labelLength-4,
                                            unicodeBuffer, fromPunycode.getCapacity(),
                                            NULL, &punycodeErrorCode);
        }
        fromPunycode.releaseBuffer(unicodeLength);
        if(U_FAILURE(punycodeErrorCode)) {
            info.labelErrors|=UIDNA_ERROR_PUNYCODE;
            return markBadACELabel(dest, labelStart, labelLength, toASCII, info);
        }
        
        
        
        
        
        
        
        UBool isValid=uts46Norm2.isNormalized(fromPunycode, errorCode);
        if(U_FAILURE(errorCode)) {
            return labelLength;
        }
        if(!isValid) {
            info.labelErrors|=UIDNA_ERROR_INVALID_ACE_LABEL;
            return markBadACELabel(dest, labelStart, labelLength, toASCII, info);
        }
        labelString=&fromPunycode;
        label=fromPunycode.getBuffer();
        labelStart=0;
        labelLength=fromPunycode.length();
    } else {
        wasPunycode=FALSE;
        labelString=&dest;
    }
    
    if(labelLength==0) {
        if(toASCII) {
            info.labelErrors|=UIDNA_ERROR_EMPTY_LABEL;
        }
        return replaceLabel(dest, destLabelStart, destLabelLength, *labelString, labelLength);
    }
    
    if(labelLength>=4 && label[2]==0x2d && label[3]==0x2d) {
        
        info.labelErrors|=UIDNA_ERROR_HYPHEN_3_4;
    }
    if(label[0]==0x2d) {
        
        info.labelErrors|=UIDNA_ERROR_LEADING_HYPHEN;
    }
    if(label[labelLength-1]==0x2d) {
        
        info.labelErrors|=UIDNA_ERROR_TRAILING_HYPHEN;
    }
    
    
    
    
    
    
    
    
    
    UChar *s=(UChar *)label;
    const UChar *limit=label+labelLength;
    UChar oredChars=0;
    
    UBool disallowNonLDHDot=(options&UIDNA_USE_STD3_RULES)!=0;
    do {
        UChar c=*s;
        if(c<=0x7f) {
            if(c==0x2e) {
                info.labelErrors|=UIDNA_ERROR_LABEL_HAS_DOT;
                *s=0xfffd;
            } else if(disallowNonLDHDot && asciiData[c]<0) {
                info.labelErrors|=UIDNA_ERROR_DISALLOWED;
                *s=0xfffd;
            }
        } else {
            oredChars|=c;
            if(disallowNonLDHDot && isNonASCIIDisallowedSTD3Valid(c)) {
                info.labelErrors|=UIDNA_ERROR_DISALLOWED;
                *s=0xfffd;
            } else if(c==0xfffd) {
                info.labelErrors|=UIDNA_ERROR_DISALLOWED;
            }
        }
        ++s;
    } while(s<limit);
    
    
    UChar32 c;
    int32_t cpLength=0;
    
    U16_NEXT_UNSAFE(label, cpLength, c);
    if((U_GET_GC_MASK(c)&U_GC_M_MASK)!=0) {
        info.labelErrors|=UIDNA_ERROR_LEADING_COMBINING_MARK;
        labelString->replace(labelStart, cpLength, (UChar)0xfffd);
        label=labelString->getBuffer()+labelStart;
        labelLength+=1-cpLength;
        if(labelString==&dest) {
            destLabelLength=labelLength;
        }
    }
    if((info.labelErrors&severeErrors)==0) {
        
        
        if((options&UIDNA_CHECK_BIDI)!=0 && (!info.isBiDi || info.isOkBiDi)) {
            checkLabelBiDi(label, labelLength, info);
        }
        if( (options&UIDNA_CHECK_CONTEXTJ)!=0 && (oredChars&0x200c)==0x200c &&
            !isLabelOkContextJ(label, labelLength)
        ) {
            info.labelErrors|=UIDNA_ERROR_CONTEXTJ;
        }
        if((options&UIDNA_CHECK_CONTEXTO)!=0 && oredChars>=0xb7) {
            checkLabelContextO(label, labelLength, info);
        }
        if(toASCII) {
            if(wasPunycode) {
                
                if(destLabelLength>63) {
                    info.labelErrors|=UIDNA_ERROR_LABEL_TOO_LONG;
                }
                return destLabelLength;
            } else if(oredChars>=0x80) {
                
                UnicodeString punycode;
                UChar *buffer=punycode.getBuffer(63);  
                if(buffer==NULL) {
                    errorCode=U_MEMORY_ALLOCATION_ERROR;
                    return destLabelLength;
                }
                buffer[0]=0x78;  
                buffer[1]=0x6e;
                buffer[2]=0x2d;
                buffer[3]=0x2d;
                int32_t punycodeLength=u_strToPunycode(label, labelLength,
                                                      buffer+4, punycode.getCapacity()-4,
                                                      NULL, &errorCode);
                if(errorCode==U_BUFFER_OVERFLOW_ERROR) {
                    errorCode=U_ZERO_ERROR;
                    punycode.releaseBuffer(4);
                    buffer=punycode.getBuffer(4+punycodeLength);
                    if(buffer==NULL) {
                        errorCode=U_MEMORY_ALLOCATION_ERROR;
                        return destLabelLength;
                    }
                    punycodeLength=u_strToPunycode(label, labelLength,
                                                  buffer+4, punycode.getCapacity()-4,
                                                  NULL, &errorCode);
                }
                punycodeLength+=4;
                punycode.releaseBuffer(punycodeLength);
                if(U_FAILURE(errorCode)) {
                    return destLabelLength;
                }
                if(punycodeLength>63) {
                    info.labelErrors|=UIDNA_ERROR_LABEL_TOO_LONG;
                }
                return replaceLabel(dest, destLabelStart, destLabelLength,
                                    punycode, punycodeLength);
            } else {
                
                if(labelLength>63) {
                    info.labelErrors|=UIDNA_ERROR_LABEL_TOO_LONG;
                }
            }
        }
    } else {
        
        
        if(wasPunycode) {
            info.labelErrors|=UIDNA_ERROR_INVALID_ACE_LABEL;
            return markBadACELabel(dest, destLabelStart, destLabelLength, toASCII, info);
        }
    }
    return replaceLabel(dest, destLabelStart, destLabelLength, *labelString, labelLength);
}




int32_t
UTS46::markBadACELabel(UnicodeString &dest,
                       int32_t labelStart, int32_t labelLength,
                       UBool toASCII, IDNAInfo &info) const {
    UBool disallowNonLDHDot=(options&UIDNA_USE_STD3_RULES)!=0;
    UBool isASCII=TRUE;
    UBool onlyLDH=TRUE;
    const UChar *label=dest.getBuffer()+labelStart;
    
    UChar *s=(UChar *)label+4;  
    const UChar *limit=label+labelLength;
    do {
        UChar c=*s;
        if(c<=0x7f) {
            if(c==0x2e) {
                info.labelErrors|=UIDNA_ERROR_LABEL_HAS_DOT;
                *s=0xfffd;
                isASCII=onlyLDH=FALSE;
            } else if(asciiData[c]<0) {
                onlyLDH=FALSE;
                if(disallowNonLDHDot) {
                    *s=0xfffd;
                    isASCII=FALSE;
                }
            }
        } else {
            isASCII=onlyLDH=FALSE;
        }
    } while(++s<limit);
    if(onlyLDH) {
        dest.insert(labelStart+labelLength, (UChar)0xfffd);
        ++labelLength;
    } else {
        if(toASCII && isASCII && labelLength>63) {
            info.labelErrors|=UIDNA_ERROR_LABEL_TOO_LONG;
        }
    }
    return labelLength;
}

const uint32_t L_MASK=U_MASK(U_LEFT_TO_RIGHT);
const uint32_t R_AL_MASK=U_MASK(U_RIGHT_TO_LEFT)|U_MASK(U_RIGHT_TO_LEFT_ARABIC);
const uint32_t L_R_AL_MASK=L_MASK|R_AL_MASK;

const uint32_t R_AL_AN_MASK=R_AL_MASK|U_MASK(U_ARABIC_NUMBER);

const uint32_t EN_AN_MASK=U_MASK(U_EUROPEAN_NUMBER)|U_MASK(U_ARABIC_NUMBER);
const uint32_t R_AL_EN_AN_MASK=R_AL_MASK|EN_AN_MASK;
const uint32_t L_EN_MASK=L_MASK|U_MASK(U_EUROPEAN_NUMBER);

const uint32_t ES_CS_ET_ON_BN_NSM_MASK=
    U_MASK(U_EUROPEAN_NUMBER_SEPARATOR)|
    U_MASK(U_COMMON_NUMBER_SEPARATOR)|
    U_MASK(U_EUROPEAN_NUMBER_TERMINATOR)|
    U_MASK(U_OTHER_NEUTRAL)|
    U_MASK(U_BOUNDARY_NEUTRAL)|
    U_MASK(U_DIR_NON_SPACING_MARK);
const uint32_t L_EN_ES_CS_ET_ON_BN_NSM_MASK=L_EN_MASK|ES_CS_ET_ON_BN_NSM_MASK;
const uint32_t R_AL_AN_EN_ES_CS_ET_ON_BN_NSM_MASK=R_AL_MASK|EN_AN_MASK|ES_CS_ET_ON_BN_NSM_MASK;






void
UTS46::checkLabelBiDi(const UChar *label, int32_t labelLength, IDNAInfo &info) const {
    
    
    UChar32 c;
    int32_t i=0;
    U16_NEXT_UNSAFE(label, i, c);
    uint32_t firstMask=U_MASK(u_charDirection(c));
    
    
    
    if((firstMask&~L_R_AL_MASK)!=0) {
        info.isOkBiDi=FALSE;
    }
    
    uint32_t lastMask;
    for(;;) {
        if(i>=labelLength) {
            lastMask=firstMask;
            break;
        }
        U16_PREV_UNSAFE(label, labelLength, c);
        UCharDirection dir=u_charDirection(c);
        if(dir!=U_DIR_NON_SPACING_MARK) {
            lastMask=U_MASK(dir);
            break;
        }
    }
    
    
    
    
    
    
    if( (firstMask&L_MASK)!=0 ?
            (lastMask&~L_EN_MASK)!=0 :
            (lastMask&~R_AL_EN_AN_MASK)!=0
    ) {
        info.isOkBiDi=FALSE;
    }
    
    uint32_t mask=0;
    while(i<labelLength) {
        U16_NEXT_UNSAFE(label, i, c);
        mask|=U_MASK(u_charDirection(c));
    }
    if(firstMask&L_MASK) {
        
        
        if((mask&~L_EN_ES_CS_ET_ON_BN_NSM_MASK)!=0) {
            info.isOkBiDi=FALSE;
        }
    } else {
        
        
        if((mask&~R_AL_AN_EN_ES_CS_ET_ON_BN_NSM_MASK)!=0) {
            info.isOkBiDi=FALSE;
        }
        
        
        if((mask&EN_AN_MASK)==EN_AN_MASK) {
            info.isOkBiDi=FALSE;
        }
    }
    
    
    
    
    
    
    if(((firstMask|mask|lastMask)&R_AL_AN_MASK)!=0) {
        info.isBiDi=TRUE;
    }
}














static UBool
isASCIIOkBiDi(const UChar *s, int32_t length) {
    int32_t labelStart=0;
    for(int32_t i=0; i<length; ++i) {
        UChar c=s[i];
        if(c==0x2e) {  
            if(i>labelStart) {
                c=s[i-1];
                if(!(0x61<=c && c<=0x7a) && !(0x30<=c && c<=0x39)) {
                    
                    return FALSE;
                }
            }
            labelStart=i+1;
        } else if(i==labelStart) {
            if(!(0x61<=c && c<=0x7a)) {
                
                return FALSE;
            }
        } else {
            if(c<=0x20 && (c>=0x1c || (9<=c && c<=0xd))) {
                
                return FALSE;
            }
        }
    }
    return TRUE;
}




static UBool
isASCIIOkBiDi(const char *s, int32_t length) {
    int32_t labelStart=0;
    for(int32_t i=0; i<length; ++i) {
        char c=s[i];
        if(c==0x2e) {  
            if(i>labelStart) {
                c=s[i-1];
                if(!(0x61<=c && c<=0x7a) && !(0x41<=c && c<=0x5a) && !(0x30<=c && c<=0x39)) {
                    
                    return FALSE;
                }
            }
            labelStart=i+1;
        } else if(i==labelStart) {
            if(!(0x61<=c && c<=0x7a) && !(0x41<=c && c<=0x5a)) {
                
                return FALSE;
            }
        } else {
            if(c<=0x20 && (c>=0x1c || (9<=c && c<=0xd))) {
                
                return FALSE;
            }
        }
    }
    return TRUE;
}

UBool
UTS46::isLabelOkContextJ(const UChar *label, int32_t labelLength) const {
    const UBiDiProps *bdp=ubidi_getSingleton();
    
    
    for(int32_t i=0; i<labelLength; ++i) {
        if(label[i]==0x200c) {
            
            
            
            
            
            
            if(i==0) {
                return FALSE;
            }
            UChar32 c;
            int32_t j=i;
            U16_PREV_UNSAFE(label, j, c);
            if(uts46Norm2.getCombiningClass(c)==9) {
                continue;
            }
            
            for(;;) {
                UJoiningType type=ubidi_getJoiningType(bdp, c);
                if(type==U_JT_TRANSPARENT) {
                    if(j==0) {
                        return FALSE;
                    }
                    U16_PREV_UNSAFE(label, j, c);
                } else if(type==U_JT_LEFT_JOINING || type==U_JT_DUAL_JOINING) {
                    break;  
                } else {
                    return FALSE;
                }
            }
            
            for(j=i+1;;) {
                if(j==labelLength) {
                    return FALSE;
                }
                U16_NEXT_UNSAFE(label, j, c);
                UJoiningType type=ubidi_getJoiningType(bdp, c);
                if(type==U_JT_TRANSPARENT) {
                    
                } else if(type==U_JT_RIGHT_JOINING || type==U_JT_DUAL_JOINING) {
                    break;  
                } else {
                    return FALSE;
                }
            }
        } else if(label[i]==0x200d) {
            
            
            
            
            if(i==0) {
                return FALSE;
            }
            UChar32 c;
            int32_t j=i;
            U16_PREV_UNSAFE(label, j, c);
            if(uts46Norm2.getCombiningClass(c)!=9) {
                return FALSE;
            }
        }
    }
    return TRUE;
}

void
UTS46::checkLabelContextO(const UChar *label, int32_t labelLength, IDNAInfo &info) const {
    int32_t labelEnd=labelLength-1;  
    int32_t arabicDigits=0;  
    for(int32_t i=0; i<=labelEnd; ++i) {
        UChar32 c=label[i];
        if(c<0xb7) {
            
        } else if(c<=0x6f9) {
            if(c==0xb7) {
                
                
                
                
                
                if(!(0<i && label[i-1]==0x6c &&
                     i<labelEnd && label[i+1]==0x6c)) {
                    info.labelErrors|=UIDNA_ERROR_CONTEXTO_PUNCTUATION;
                }
            } else if(c==0x375) {
                
                
                
                
                UScriptCode script=USCRIPT_INVALID_CODE;
                if(i<labelEnd) {
                    UErrorCode errorCode=U_ZERO_ERROR;
                    int32_t j=i+1;
                    U16_NEXT(label, j, labelLength, c);
                    script=uscript_getScript(c, &errorCode);
                }
                if(script!=USCRIPT_GREEK) {
                    info.labelErrors|=UIDNA_ERROR_CONTEXTO_PUNCTUATION;
                }
            } else if(c==0x5f3 || c==0x5f4) {
                
                
                
                
                
                
                
                
                
                UScriptCode script=USCRIPT_INVALID_CODE;
                if(0<i) {
                    UErrorCode errorCode=U_ZERO_ERROR;
                    int32_t j=i;
                    U16_PREV(label, 0, j, c);
                    script=uscript_getScript(c, &errorCode);
                }
                if(script!=USCRIPT_HEBREW) {
                    info.labelErrors|=UIDNA_ERROR_CONTEXTO_PUNCTUATION;
                }
            } else if(0x660<=c ) {
                
                
                
                
                
                
                
                
                
                
                
                
                
                if(c<=0x669) {
                    if(arabicDigits>0) {
                        info.labelErrors|=UIDNA_ERROR_CONTEXTO_DIGITS;
                    }
                    arabicDigits=-1;
                } else if(0x6f0<=c) {
                    if(arabicDigits<0) {
                        info.labelErrors|=UIDNA_ERROR_CONTEXTO_DIGITS;
                    }
                    arabicDigits=1;
                }
            }
        } else if(c==0x30fb) {
            
            
            
            
            
            
            UErrorCode errorCode=U_ZERO_ERROR;
            for(int j=0;;) {
                if(j>labelEnd) {
                    info.labelErrors|=UIDNA_ERROR_CONTEXTO_PUNCTUATION;
                    break;
                }
                U16_NEXT(label, j, labelLength, c);
                UScriptCode script=uscript_getScript(c, &errorCode);
                if(script==USCRIPT_HIRAGANA || script==USCRIPT_KATAKANA || script==USCRIPT_HAN) {
                    break;
                }
            }
        }
    }
}

U_NAMESPACE_END



U_NAMESPACE_USE

U_CAPI UIDNA * U_EXPORT2
uidna_openUTS46(uint32_t options, UErrorCode *pErrorCode) {
    return reinterpret_cast<UIDNA *>(IDNA::createUTS46Instance(options, *pErrorCode));
}

U_CAPI void U_EXPORT2
uidna_close(UIDNA *idna) {
    delete reinterpret_cast<IDNA *>(idna);
}

static UBool
checkArgs(const void *label, int32_t length,
          void *dest, int32_t capacity,
          UIDNAInfo *pInfo, UErrorCode *pErrorCode) {
    if(U_FAILURE(*pErrorCode)) {
        return FALSE;
    }
    
    if(pInfo==NULL || pInfo->size<16) {
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return FALSE;
    }
    if( (label==NULL ? length!=0 : length<-1) ||
        (dest==NULL ? capacity!=0 : capacity<0) ||
        (dest==label && label!=NULL)
    ) {
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return FALSE;
    }
    
    uprv_memset(&pInfo->size+1, 0, pInfo->size-sizeof(pInfo->size));
    return TRUE;
}

static void
idnaInfoToStruct(IDNAInfo &info, UIDNAInfo *pInfo) {
    pInfo->isTransitionalDifferent=info.isTransitionalDifferent();
    pInfo->errors=info.getErrors();
}

U_CAPI int32_t U_EXPORT2
uidna_labelToASCII(const UIDNA *idna,
                   const UChar *label, int32_t length,
                   UChar *dest, int32_t capacity,
                   UIDNAInfo *pInfo, UErrorCode *pErrorCode) {
    if(!checkArgs(label, length, dest, capacity, pInfo, pErrorCode)) {
        return 0;
    }
    UnicodeString src((UBool)(length<0), label, length);
    UnicodeString destString(dest, 0, capacity);
    IDNAInfo info;
    reinterpret_cast<const IDNA *>(idna)->labelToASCII(src, destString, info, *pErrorCode);
    idnaInfoToStruct(info, pInfo);
    return destString.extract(dest, capacity, *pErrorCode);
}

U_CAPI int32_t U_EXPORT2
uidna_labelToUnicode(const UIDNA *idna,
                     const UChar *label, int32_t length,
                     UChar *dest, int32_t capacity,
                     UIDNAInfo *pInfo, UErrorCode *pErrorCode) {
    if(!checkArgs(label, length, dest, capacity, pInfo, pErrorCode)) {
        return 0;
    }
    UnicodeString src((UBool)(length<0), label, length);
    UnicodeString destString(dest, 0, capacity);
    IDNAInfo info;
    reinterpret_cast<const IDNA *>(idna)->labelToUnicode(src, destString, info, *pErrorCode);
    idnaInfoToStruct(info, pInfo);
    return destString.extract(dest, capacity, *pErrorCode);
}

U_CAPI int32_t U_EXPORT2
uidna_nameToASCII(const UIDNA *idna,
                  const UChar *name, int32_t length,
                  UChar *dest, int32_t capacity,
                  UIDNAInfo *pInfo, UErrorCode *pErrorCode) {
    if(!checkArgs(name, length, dest, capacity, pInfo, pErrorCode)) {
        return 0;
    }
    UnicodeString src((UBool)(length<0), name, length);
    UnicodeString destString(dest, 0, capacity);
    IDNAInfo info;
    reinterpret_cast<const IDNA *>(idna)->nameToASCII(src, destString, info, *pErrorCode);
    idnaInfoToStruct(info, pInfo);
    return destString.extract(dest, capacity, *pErrorCode);
}

U_CAPI int32_t U_EXPORT2
uidna_nameToUnicode(const UIDNA *idna,
                    const UChar *name, int32_t length,
                    UChar *dest, int32_t capacity,
                    UIDNAInfo *pInfo, UErrorCode *pErrorCode) {
    if(!checkArgs(name, length, dest, capacity, pInfo, pErrorCode)) {
        return 0;
    }
    UnicodeString src((UBool)(length<0), name, length);
    UnicodeString destString(dest, 0, capacity);
    IDNAInfo info;
    reinterpret_cast<const IDNA *>(idna)->nameToUnicode(src, destString, info, *pErrorCode);
    idnaInfoToStruct(info, pInfo);
    return destString.extract(dest, capacity, *pErrorCode);
}

U_CAPI int32_t U_EXPORT2
uidna_labelToASCII_UTF8(const UIDNA *idna,
                        const char *label, int32_t length,
                        char *dest, int32_t capacity,
                        UIDNAInfo *pInfo, UErrorCode *pErrorCode) {
    if(!checkArgs(label, length, dest, capacity, pInfo, pErrorCode)) {
        return 0;
    }
    StringPiece src(label, length<0 ? uprv_strlen(label) : length);
    CheckedArrayByteSink sink(dest, capacity);
    IDNAInfo info;
    reinterpret_cast<const IDNA *>(idna)->labelToASCII_UTF8(src, sink, info, *pErrorCode);
    idnaInfoToStruct(info, pInfo);
    return u_terminateChars(dest, capacity, sink.NumberOfBytesAppended(), pErrorCode);
}

U_CAPI int32_t U_EXPORT2
uidna_labelToUnicodeUTF8(const UIDNA *idna,
                         const char *label, int32_t length,
                         char *dest, int32_t capacity,
                         UIDNAInfo *pInfo, UErrorCode *pErrorCode) {
    if(!checkArgs(label, length, dest, capacity, pInfo, pErrorCode)) {
        return 0;
    }
    StringPiece src(label, length<0 ? uprv_strlen(label) : length);
    CheckedArrayByteSink sink(dest, capacity);
    IDNAInfo info;
    reinterpret_cast<const IDNA *>(idna)->labelToUnicodeUTF8(src, sink, info, *pErrorCode);
    idnaInfoToStruct(info, pInfo);
    return u_terminateChars(dest, capacity, sink.NumberOfBytesAppended(), pErrorCode);
}

U_CAPI int32_t U_EXPORT2
uidna_nameToASCII_UTF8(const UIDNA *idna,
                       const char *name, int32_t length,
                       char *dest, int32_t capacity,
                       UIDNAInfo *pInfo, UErrorCode *pErrorCode) {
    if(!checkArgs(name, length, dest, capacity, pInfo, pErrorCode)) {
        return 0;
    }
    StringPiece src(name, length<0 ? uprv_strlen(name) : length);
    CheckedArrayByteSink sink(dest, capacity);
    IDNAInfo info;
    reinterpret_cast<const IDNA *>(idna)->nameToASCII_UTF8(src, sink, info, *pErrorCode);
    idnaInfoToStruct(info, pInfo);
    return u_terminateChars(dest, capacity, sink.NumberOfBytesAppended(), pErrorCode);
}

U_CAPI int32_t U_EXPORT2
uidna_nameToUnicodeUTF8(const UIDNA *idna,
                        const char *name, int32_t length,
                        char *dest, int32_t capacity,
                        UIDNAInfo *pInfo, UErrorCode *pErrorCode) {
    if(!checkArgs(name, length, dest, capacity, pInfo, pErrorCode)) {
        return 0;
    }
    StringPiece src(name, length<0 ? uprv_strlen(name) : length);
    CheckedArrayByteSink sink(dest, capacity);
    IDNAInfo info;
    reinterpret_cast<const IDNA *>(idna)->nameToUnicodeUTF8(src, sink, info, *pErrorCode);
    idnaInfoToStruct(info, pInfo);
    return u_terminateChars(dest, capacity, sink.NumberOfBytesAppended(), pErrorCode);
}

#endif  
