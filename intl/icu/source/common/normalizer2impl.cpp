















#include "unicode/utypes.h"

#if !UCONFIG_NO_NORMALIZATION

#include "unicode/normalizer2.h"
#include "unicode/udata.h"
#include "unicode/ustring.h"
#include "unicode/utf16.h"
#include "cmemory.h"
#include "mutex.h"
#include "normalizer2impl.h"
#include "putilimp.h"
#include "uassert.h"
#include "uset_imp.h"
#include "utrie2.h"
#include "uvector.h"

U_NAMESPACE_BEGIN



UBool ReorderingBuffer::init(int32_t destCapacity, UErrorCode &errorCode) {
    int32_t length=str.length();
    start=str.getBuffer(destCapacity);
    if(start==NULL) {
        
        errorCode=U_MEMORY_ALLOCATION_ERROR;
        return FALSE;
    }
    limit=start+length;
    remainingCapacity=str.getCapacity()-length;
    reorderStart=start;
    if(start==limit) {
        lastCC=0;
    } else {
        setIterator();
        lastCC=previousCC();
        
        if(lastCC>1) {
            while(previousCC()>1) {}
        }
        reorderStart=codePointLimit;
    }
    return TRUE;
}

UBool ReorderingBuffer::equals(const UChar *otherStart, const UChar *otherLimit) const {
    int32_t length=(int32_t)(limit-start);
    return
        length==(int32_t)(otherLimit-otherStart) &&
        0==u_memcmp(start, otherStart, length);
}

UBool ReorderingBuffer::appendSupplementary(UChar32 c, uint8_t cc, UErrorCode &errorCode) {
    if(remainingCapacity<2 && !resize(2, errorCode)) {
        return FALSE;
    }
    if(lastCC<=cc || cc==0) {
        limit[0]=U16_LEAD(c);
        limit[1]=U16_TRAIL(c);
        limit+=2;
        lastCC=cc;
        if(cc<=1) {
            reorderStart=limit;
        }
    } else {
        insert(c, cc);
    }
    remainingCapacity-=2;
    return TRUE;
}

UBool ReorderingBuffer::append(const UChar *s, int32_t length,
                               uint8_t leadCC, uint8_t trailCC,
                               UErrorCode &errorCode) {
    if(length==0) {
        return TRUE;
    }
    if(remainingCapacity<length && !resize(length, errorCode)) {
        return FALSE;
    }
    remainingCapacity-=length;
    if(lastCC<=leadCC || leadCC==0) {
        if(trailCC<=1) {
            reorderStart=limit+length;
        } else if(leadCC<=1) {
            reorderStart=limit+1;  
        }
        const UChar *sLimit=s+length;
        do { *limit++=*s++; } while(s!=sLimit);
        lastCC=trailCC;
    } else {
        int32_t i=0;
        UChar32 c;
        U16_NEXT(s, i, length, c);
        insert(c, leadCC);  
        while(i<length) {
            U16_NEXT(s, i, length, c);
            if(i<length) {
                
                leadCC=Normalizer2Impl::getCCFromYesOrMaybe(impl.getNorm16(c));
            } else {
                leadCC=trailCC;
            }
            append(c, leadCC, errorCode);
        }
    }
    return TRUE;
}

UBool ReorderingBuffer::appendZeroCC(UChar32 c, UErrorCode &errorCode) {
    int32_t cpLength=U16_LENGTH(c);
    if(remainingCapacity<cpLength && !resize(cpLength, errorCode)) {
        return FALSE;
    }
    remainingCapacity-=cpLength;
    if(cpLength==1) {
        *limit++=(UChar)c;
    } else {
        limit[0]=U16_LEAD(c);
        limit[1]=U16_TRAIL(c);
        limit+=2;
    }
    lastCC=0;
    reorderStart=limit;
    return TRUE;
}

UBool ReorderingBuffer::appendZeroCC(const UChar *s, const UChar *sLimit, UErrorCode &errorCode) {
    if(s==sLimit) {
        return TRUE;
    }
    int32_t length=(int32_t)(sLimit-s);
    if(remainingCapacity<length && !resize(length, errorCode)) {
        return FALSE;
    }
    u_memcpy(limit, s, length);
    limit+=length;
    remainingCapacity-=length;
    lastCC=0;
    reorderStart=limit;
    return TRUE;
}

void ReorderingBuffer::remove() {
    reorderStart=limit=start;
    remainingCapacity=str.getCapacity();
    lastCC=0;
}

void ReorderingBuffer::removeSuffix(int32_t suffixLength) {
    if(suffixLength<(limit-start)) {
        limit-=suffixLength;
        remainingCapacity+=suffixLength;
    } else {
        limit=start;
        remainingCapacity=str.getCapacity();
    }
    lastCC=0;
    reorderStart=limit;
}

UBool ReorderingBuffer::resize(int32_t appendLength, UErrorCode &errorCode) {
    int32_t reorderStartIndex=(int32_t)(reorderStart-start);
    int32_t length=(int32_t)(limit-start);
    str.releaseBuffer(length);
    int32_t newCapacity=length+appendLength;
    int32_t doubleCapacity=2*str.getCapacity();
    if(newCapacity<doubleCapacity) {
        newCapacity=doubleCapacity;
    }
    if(newCapacity<256) {
        newCapacity=256;
    }
    start=str.getBuffer(newCapacity);
    if(start==NULL) {
        
        errorCode=U_MEMORY_ALLOCATION_ERROR;
        return FALSE;
    }
    reorderStart=start+reorderStartIndex;
    limit=start+length;
    remainingCapacity=str.getCapacity()-length;
    return TRUE;
}

void ReorderingBuffer::skipPrevious() {
    codePointLimit=codePointStart;
    UChar c=*--codePointStart;
    if(U16_IS_TRAIL(c) && start<codePointStart && U16_IS_LEAD(*(codePointStart-1))) {
        --codePointStart;
    }
}

uint8_t ReorderingBuffer::previousCC() {
    codePointLimit=codePointStart;
    if(reorderStart>=codePointStart) {
        return 0;
    }
    UChar32 c=*--codePointStart;
    if(c<Normalizer2Impl::MIN_CCC_LCCC_CP) {
        return 0;
    }

    UChar c2;
    if(U16_IS_TRAIL(c) && start<codePointStart && U16_IS_LEAD(c2=*(codePointStart-1))) {
        --codePointStart;
        c=U16_GET_SUPPLEMENTARY(c2, c);
    }
    return Normalizer2Impl::getCCFromYesOrMaybe(impl.getNorm16(c));
}



void ReorderingBuffer::insert(UChar32 c, uint8_t cc) {
    for(setIterator(), skipPrevious(); previousCC()>cc;) {}
    
    UChar *q=limit;
    UChar *r=limit+=U16_LENGTH(c);
    do {
        *--r=*--q;
    } while(codePointLimit!=q);
    writeCodePoint(q, c);
    if(cc<=1) {
        reorderStart=r;
    }
}



struct CanonIterData : public UMemory {
    CanonIterData(UErrorCode &errorCode);
    ~CanonIterData();
    void addToStartSet(UChar32 origin, UChar32 decompLead, UErrorCode &errorCode);
    UTrie2 *trie;
    UVector canonStartSets;  
};

Normalizer2Impl::~Normalizer2Impl() {
    udata_close(memory);
    utrie2_close(normTrie);
    delete (CanonIterData *)canonIterDataSingleton.fInstance;
}

UBool U_CALLCONV
Normalizer2Impl::isAcceptable(void *context,
                              const char * , const char * ,
                              const UDataInfo *pInfo) {
    if(
        pInfo->size>=20 &&
        pInfo->isBigEndian==U_IS_BIG_ENDIAN &&
        pInfo->charsetFamily==U_CHARSET_FAMILY &&
        pInfo->dataFormat[0]==0x4e &&    
        pInfo->dataFormat[1]==0x72 &&
        pInfo->dataFormat[2]==0x6d &&
        pInfo->dataFormat[3]==0x32 &&
        pInfo->formatVersion[0]==2
    ) {
        Normalizer2Impl *me=(Normalizer2Impl *)context;
        uprv_memcpy(me->dataVersion, pInfo->dataVersion, 4);
        return TRUE;
    } else {
        return FALSE;
    }
}

void
Normalizer2Impl::load(const char *packageName, const char *name, UErrorCode &errorCode) {
    if(U_FAILURE(errorCode)) {
        return;
    }
    memory=udata_openChoice(packageName, "nrm", name, isAcceptable, this, &errorCode);
    if(U_FAILURE(errorCode)) {
        return;
    }
    const uint8_t *inBytes=(const uint8_t *)udata_getMemory(memory);
    const int32_t *inIndexes=(const int32_t *)inBytes;
    int32_t indexesLength=inIndexes[IX_NORM_TRIE_OFFSET]/4;
    if(indexesLength<=IX_MIN_MAYBE_YES) {
        errorCode=U_INVALID_FORMAT_ERROR;  
        return;
    }

    minDecompNoCP=inIndexes[IX_MIN_DECOMP_NO_CP];
    minCompNoMaybeCP=inIndexes[IX_MIN_COMP_NO_MAYBE_CP];

    minYesNo=inIndexes[IX_MIN_YES_NO];
    minYesNoMappingsOnly=inIndexes[IX_MIN_YES_NO_MAPPINGS_ONLY];
    minNoNo=inIndexes[IX_MIN_NO_NO];
    limitNoNo=inIndexes[IX_LIMIT_NO_NO];
    minMaybeYes=inIndexes[IX_MIN_MAYBE_YES];

    int32_t offset=inIndexes[IX_NORM_TRIE_OFFSET];
    int32_t nextOffset=inIndexes[IX_EXTRA_DATA_OFFSET];
    normTrie=utrie2_openFromSerialized(UTRIE2_16_VALUE_BITS,
                                       inBytes+offset, nextOffset-offset, NULL,
                                       &errorCode);
    if(U_FAILURE(errorCode)) {
        return;
    }

    offset=nextOffset;
    nextOffset=inIndexes[IX_SMALL_FCD_OFFSET];
    maybeYesCompositions=(const uint16_t *)(inBytes+offset);
    extraData=maybeYesCompositions+(MIN_NORMAL_MAYBE_YES-minMaybeYes);

    
    offset=nextOffset;
    smallFCD=inBytes+offset;

    
    
    uint8_t bits=0;
    for(UChar c=0; c<0x180; bits>>=1) {
        if((c&0xff)==0) {
            bits=smallFCD[c>>8];  
        }
        if(bits&1) {
            for(int i=0; i<0x20; ++i, ++c) {
                tccc180[c]=(uint8_t)getFCD16FromNormData(c);
            }
        } else {
            uprv_memset(tccc180+c, 0, 0x20);
            c+=0x20;
        }
    }
}

uint8_t Normalizer2Impl::getTrailCCFromCompYesAndZeroCC(const UChar *cpStart, const UChar *cpLimit) const {
    UChar32 c;
    if(cpStart==(cpLimit-1)) {
        c=*cpStart;
    } else {
        c=U16_GET_SUPPLEMENTARY(cpStart[0], cpStart[1]);
    }
    uint16_t prevNorm16=getNorm16(c);
    if(prevNorm16<=minYesNo) {
        return 0;  
    } else {
        return (uint8_t)(*getMapping(prevNorm16)>>8);  
    }
}

U_CDECL_BEGIN

static UBool U_CALLCONV
enumPropertyStartsRange(const void *context, UChar32 start, UChar32 , uint32_t ) {
    
    const USetAdder *sa=(const USetAdder *)context;
    sa->add(sa->set, start);
    return TRUE;
}

static uint32_t U_CALLCONV
segmentStarterMapper(const void * , uint32_t value) {
    return value&CANON_NOT_SEGMENT_STARTER;
}

U_CDECL_END

void
Normalizer2Impl::addPropertyStarts(const USetAdder *sa, UErrorCode & ) const {
    
    utrie2_enum(normTrie, NULL, enumPropertyStartsRange, sa);

    
    for(UChar c=Hangul::HANGUL_BASE; c<Hangul::HANGUL_LIMIT; c+=Hangul::JAMO_T_COUNT) {
        sa->add(sa->set, c);
        sa->add(sa->set, c+1);
    }
    sa->add(sa->set, Hangul::HANGUL_LIMIT); 
}

void
Normalizer2Impl::addCanonIterPropertyStarts(const USetAdder *sa, UErrorCode &errorCode) const {
    
    if(ensureCanonIterData(errorCode)) {
        
        utrie2_enum(((CanonIterData *)canonIterDataSingleton.fInstance)->trie,
                    segmentStarterMapper, enumPropertyStartsRange, sa);
    }
}

const UChar *
Normalizer2Impl::copyLowPrefixFromNulTerminated(const UChar *src,
                                                UChar32 minNeedDataCP,
                                                ReorderingBuffer *buffer,
                                                UErrorCode &errorCode) const {
    
    
    
    
    
    const UChar *prevSrc=src;
    UChar c;
    while((c=*src++)<minNeedDataCP && c!=0) {}
    
    
    if(--src!=prevSrc) {
        if(buffer!=NULL) {
            buffer->appendZeroCC(prevSrc, src, errorCode);
        }
    }
    return src;
}




const UChar *
Normalizer2Impl::decompose(const UChar *src, const UChar *limit,
                           ReorderingBuffer *buffer,
                           UErrorCode &errorCode) const {
    UChar32 minNoCP=minDecompNoCP;
    if(limit==NULL) {
        src=copyLowPrefixFromNulTerminated(src, minNoCP, buffer, errorCode);
        if(U_FAILURE(errorCode)) {
            return src;
        }
        limit=u_strchr(src, 0);
    }

    const UChar *prevSrc;
    UChar32 c=0;
    uint16_t norm16=0;

    
    const UChar *prevBoundary=src;
    uint8_t prevCC=0;

    for(;;) {
        
        for(prevSrc=src; src!=limit;) {
            if( (c=*src)<minNoCP ||
                isMostDecompYesAndZeroCC(norm16=UTRIE2_GET16_FROM_U16_SINGLE_LEAD(normTrie, c))
            ) {
                ++src;
            } else if(!U16_IS_SURROGATE(c)) {
                break;
            } else {
                UChar c2;
                if(U16_IS_SURROGATE_LEAD(c)) {
                    if((src+1)!=limit && U16_IS_TRAIL(c2=src[1])) {
                        c=U16_GET_SUPPLEMENTARY(c, c2);
                    }
                } else  {
                    if(prevSrc<src && U16_IS_LEAD(c2=*(src-1))) {
                        --src;
                        c=U16_GET_SUPPLEMENTARY(c2, c);
                    }
                }
                if(isMostDecompYesAndZeroCC(norm16=getNorm16(c))) {
                    src+=U16_LENGTH(c);
                } else {
                    break;
                }
            }
        }
        
        if(src!=prevSrc) {
            if(buffer!=NULL) {
                if(!buffer->appendZeroCC(prevSrc, src, errorCode)) {
                    break;
                }
            } else {
                prevCC=0;
                prevBoundary=src;
            }
        }
        if(src==limit) {
            break;
        }

        
        src+=U16_LENGTH(c);
        if(buffer!=NULL) {
            if(!decompose(c, norm16, *buffer, errorCode)) {
                break;
            }
        } else {
            if(isDecompYes(norm16)) {
                uint8_t cc=getCCFromYesOrMaybe(norm16);
                if(prevCC<=cc || cc==0) {
                    prevCC=cc;
                    if(cc<=1) {
                        prevBoundary=src;
                    }
                    continue;
                }
            }
            return prevBoundary;  
        }
    }
    return src;
}





UBool Normalizer2Impl::decomposeShort(const UChar *src, const UChar *limit,
                                      ReorderingBuffer &buffer,
                                      UErrorCode &errorCode) const {
    while(src<limit) {
        UChar32 c;
        uint16_t norm16;
        UTRIE2_U16_NEXT16(normTrie, src, limit, c, norm16);
        if(!decompose(c, norm16, buffer, errorCode)) {
            return FALSE;
        }
    }
    return TRUE;
}

UBool Normalizer2Impl::decompose(UChar32 c, uint16_t norm16,
                                 ReorderingBuffer &buffer,
                                 UErrorCode &errorCode) const {
    
    for(;;) {
        
        if(isDecompYes(norm16)) {
            
            return buffer.append(c, getCCFromYesOrMaybe(norm16), errorCode);
        } else if(isHangul(norm16)) {
            
            UChar jamos[3];
            return buffer.appendZeroCC(jamos, jamos+Hangul::decompose(c, jamos), errorCode);
        } else if(isDecompNoAlgorithmic(norm16)) {
            c=mapAlgorithmic(c, norm16);
            norm16=getNorm16(c);
        } else {
            
            const uint16_t *mapping=getMapping(norm16);
            uint16_t firstUnit=*mapping;
            int32_t length=firstUnit&MAPPING_LENGTH_MASK;
            uint8_t leadCC, trailCC;
            trailCC=(uint8_t)(firstUnit>>8);
            if(firstUnit&MAPPING_HAS_CCC_LCCC_WORD) {
                leadCC=(uint8_t)(*(mapping-1)>>8);
            } else {
                leadCC=0;
            }
            return buffer.append((const UChar *)mapping+1, length, leadCC, trailCC, errorCode);
        }
    }
}

const UChar *
Normalizer2Impl::getDecomposition(UChar32 c, UChar buffer[4], int32_t &length) const {
    const UChar *decomp=NULL;
    uint16_t norm16;
    for(;;) {
        if(c<minDecompNoCP || isDecompYes(norm16=getNorm16(c))) {
            
            return decomp;
        } else if(isHangul(norm16)) {
            
            length=Hangul::decompose(c, buffer);
            return buffer;
        } else if(isDecompNoAlgorithmic(norm16)) {
            c=mapAlgorithmic(c, norm16);
            decomp=buffer;
            length=0;
            U16_APPEND_UNSAFE(buffer, length, c);
        } else {
            
            const uint16_t *mapping=getMapping(norm16);
            length=*mapping&MAPPING_LENGTH_MASK;
            return (const UChar *)mapping+1;
        }
    }
}





const UChar *
Normalizer2Impl::getRawDecomposition(UChar32 c, UChar buffer[30], int32_t &length) const {
    
    
    uint16_t norm16;
    if(c<minDecompNoCP || isDecompYes(norm16=getNorm16(c))) {
        
        return NULL;
    } else if(isHangul(norm16)) {
        
        Hangul::getRawDecomposition(c, buffer);
        length=2;
        return buffer;
    } else if(isDecompNoAlgorithmic(norm16)) {
        c=mapAlgorithmic(c, norm16);
        length=0;
        U16_APPEND_UNSAFE(buffer, length, c);
        return buffer;
    } else {
        
        const uint16_t *mapping=getMapping(norm16);
        uint16_t firstUnit=*mapping;
        int32_t mLength=firstUnit&MAPPING_LENGTH_MASK;  
        if(firstUnit&MAPPING_HAS_RAW_MAPPING) {
            
            
            const uint16_t *rawMapping=mapping-((firstUnit>>7)&1)-1;
            uint16_t rm0=*rawMapping;
            if(rm0<=MAPPING_LENGTH_MASK) {
                length=rm0;
                return (const UChar *)rawMapping-rm0;
            } else {
                
                buffer[0]=(UChar)rm0;
                u_memcpy(buffer+1, (const UChar *)mapping+1+2, mLength-2);
                length=mLength-1;
                return buffer;
            }
        } else {
            length=mLength;
            return (const UChar *)mapping+1;
        }
    }
}

void Normalizer2Impl::decomposeAndAppend(const UChar *src, const UChar *limit,
                                         UBool doDecompose,
                                         UnicodeString &safeMiddle,
                                         ReorderingBuffer &buffer,
                                         UErrorCode &errorCode) const {
    buffer.copyReorderableSuffixTo(safeMiddle);
    if(doDecompose) {
        decompose(src, limit, &buffer, errorCode);
        return;
    }
    
    ForwardUTrie2StringIterator iter(normTrie, src, limit);
    uint8_t firstCC, prevCC, cc;
    firstCC=prevCC=cc=getCC(iter.next16());
    while(cc!=0) {
        prevCC=cc;
        cc=getCC(iter.next16());
    };
    if(limit==NULL) {  
        limit=u_strchr(iter.codePointStart, 0);
    }

    if (buffer.append(src, (int32_t)(iter.codePointStart-src), firstCC, prevCC, errorCode)) {
        buffer.appendZeroCC(iter.codePointStart, limit, errorCode);
    }
}




UBool Normalizer2Impl::hasDecompBoundary(UChar32 c, UBool before) const {
    for(;;) {
        if(c<minDecompNoCP) {
            return TRUE;
        }
        uint16_t norm16=getNorm16(c);
        if(isHangul(norm16) || isDecompYesAndZeroCC(norm16)) {
            return TRUE;
        } else if(norm16>MIN_NORMAL_MAYBE_YES) {
            return FALSE;  
        } else if(isDecompNoAlgorithmic(norm16)) {
            c=mapAlgorithmic(c, norm16);
        } else {
            
            const uint16_t *mapping=getMapping(norm16);
            uint16_t firstUnit=*mapping;
            if((firstUnit&MAPPING_LENGTH_MASK)==0) {
                return FALSE;
            }
            if(!before) {
                
                
                if(firstUnit>0x1ff) {
                    return FALSE;  
                }
                if(firstUnit<=0xff) {
                    return TRUE;  
                }
                
            }
            
            return (firstUnit&MAPPING_HAS_CCC_LCCC_WORD)==0 || (*(mapping-1)&0xff00)==0;
        }
    }
}























int32_t Normalizer2Impl::combine(const uint16_t *list, UChar32 trail) {
    uint16_t key1, firstUnit;
    if(trail<COMP_1_TRAIL_LIMIT) {
        
        
        key1=(uint16_t)(trail<<1);
        while(key1>(firstUnit=*list)) {
            list+=2+(firstUnit&COMP_1_TRIPLE);
        }
        if(key1==(firstUnit&COMP_1_TRAIL_MASK)) {
            if(firstUnit&COMP_1_TRIPLE) {
                return ((int32_t)list[1]<<16)|list[2];
            } else {
                return list[1];
            }
        }
    } else {
        
        
        key1=(uint16_t)(COMP_1_TRAIL_LIMIT+
                        (((trail>>COMP_1_TRAIL_SHIFT))&
                          ~COMP_1_TRIPLE));
        uint16_t key2=(uint16_t)(trail<<COMP_2_TRAIL_SHIFT);
        uint16_t secondUnit;
        for(;;) {
            if(key1>(firstUnit=*list)) {
                list+=2+(firstUnit&COMP_1_TRIPLE);
            } else if(key1==(firstUnit&COMP_1_TRAIL_MASK)) {
                if(key2>(secondUnit=list[1])) {
                    if(firstUnit&COMP_1_LAST_TUPLE) {
                        break;
                    } else {
                        list+=3;
                    }
                } else if(key2==(secondUnit&COMP_2_TRAIL_MASK)) {
                    return ((int32_t)(secondUnit&~COMP_2_TRAIL_MASK)<<16)|list[2];
                } else {
                    break;
                }
            } else {
                break;
            }
        }
    }
    return -1;
}





void Normalizer2Impl::addComposites(const uint16_t *list, UnicodeSet &set) const {
    uint16_t firstUnit;
    int32_t compositeAndFwd;
    do {
        firstUnit=*list;
        if((firstUnit&COMP_1_TRIPLE)==0) {
            compositeAndFwd=list[1];
            list+=2;
        } else {
            compositeAndFwd=(((int32_t)list[1]&~COMP_2_TRAIL_MASK)<<16)|list[2];
            list+=3;
        }
        UChar32 composite=compositeAndFwd>>1;
        if((compositeAndFwd&1)!=0) {
            addComposites(getCompositionsListForComposite(getNorm16(composite)), set);
        }
        set.add(composite);
    } while((firstUnit&COMP_1_LAST_TUPLE)==0);
}











void Normalizer2Impl::recompose(ReorderingBuffer &buffer, int32_t recomposeStartIndex,
                                UBool onlyContiguous) const {
    UChar *p=buffer.getStart()+recomposeStartIndex;
    UChar *limit=buffer.getLimit();
    if(p==limit) {
        return;
    }

    UChar *starter, *pRemove, *q, *r;
    const uint16_t *compositionsList;
    UChar32 c, compositeAndFwd;
    uint16_t norm16;
    uint8_t cc, prevCC;
    UBool starterIsSupplementary;

    
    
    compositionsList=NULL;  
    starter=NULL;
    starterIsSupplementary=FALSE;
    prevCC=0;

    for(;;) {
        UTRIE2_U16_NEXT16(normTrie, p, limit, c, norm16);
        cc=getCCFromYesOrMaybe(norm16);
        if( 
            isMaybe(norm16) &&
            
            compositionsList!=NULL &&
            
            (prevCC<cc || prevCC==0)
        ) {
            if(isJamoVT(norm16)) {
                
                if(c<Hangul::JAMO_T_BASE) {
                    
                    UChar prev=(UChar)(*starter-Hangul::JAMO_L_BASE);
                    if(prev<Hangul::JAMO_L_COUNT) {
                        pRemove=p-1;
                        UChar syllable=(UChar)
                            (Hangul::HANGUL_BASE+
                             (prev*Hangul::JAMO_V_COUNT+(c-Hangul::JAMO_V_BASE))*
                             Hangul::JAMO_T_COUNT);
                        UChar t;
                        if(p!=limit && (t=(UChar)(*p-Hangul::JAMO_T_BASE))<Hangul::JAMO_T_COUNT) {
                            ++p;
                            syllable+=t;  
                        }
                        *starter=syllable;
                        
                        q=pRemove;
                        r=p;
                        while(r<limit) {
                            *q++=*r++;
                        }
                        limit=q;
                        p=pRemove;
                    }
                }
                





                if(p==limit) {
                    break;
                }
                compositionsList=NULL;
                continue;
            } else if((compositeAndFwd=combine(compositionsList, c))>=0) {
                
                UChar32 composite=compositeAndFwd>>1;

                
                pRemove=p-U16_LENGTH(c);  
                if(starterIsSupplementary) {
                    if(U_IS_SUPPLEMENTARY(composite)) {
                        
                        starter[0]=U16_LEAD(composite);
                        starter[1]=U16_TRAIL(composite);
                    } else {
                        *starter=(UChar)composite;
                        
                        
                        starterIsSupplementary=FALSE;
                        q=starter+1;
                        r=q+1;
                        while(r<pRemove) {
                            *q++=*r++;
                        }
                        --pRemove;
                    }
                } else if(U_IS_SUPPLEMENTARY(composite)) {
                    
                    
                    starterIsSupplementary=TRUE;
                    ++starter;  
                    q=pRemove;
                    r=++pRemove;
                    while(starter<q) {
                        *--r=*--q;
                    }
                    *starter=U16_TRAIL(composite);
                    *--starter=U16_LEAD(composite);  
                } else {
                    
                    *starter=(UChar)composite;
                }

                
                if(pRemove<p) {
                    q=pRemove;
                    r=p;
                    while(r<limit) {
                        *q++=*r++;
                    }
                    limit=q;
                    p=pRemove;
                }
                

                if(p==limit) {
                    break;
                }
                
                if(compositeAndFwd&1) {
                    compositionsList=
                        getCompositionsListForComposite(getNorm16(composite));
                } else {
                    compositionsList=NULL;
                }

                
                continue;
            }
        }

        
        prevCC=cc;
        if(p==limit) {
            break;
        }

        
        if(cc==0) {
            
            if((compositionsList=getCompositionsListForDecompYes(norm16))!=NULL) {
                
                if(U_IS_BMP(c)) {
                    starterIsSupplementary=FALSE;
                    starter=p-1;
                } else {
                    starterIsSupplementary=TRUE;
                    starter=p-2;
                }
            }
        } else if(onlyContiguous) {
            
            compositionsList=NULL;
        }
    }
    buffer.setReorderingLimit(limit);
}

UChar32
Normalizer2Impl::composePair(UChar32 a, UChar32 b) const {
    uint16_t norm16=getNorm16(a);  
    const uint16_t *list;
    if(isInert(norm16)) {
        return U_SENTINEL;
    } else if(norm16<minYesNoMappingsOnly) {
        if(isJamoL(norm16)) {
            b-=Hangul::JAMO_V_BASE;
            if(0<=b && b<Hangul::JAMO_V_COUNT) {
                return
                    (Hangul::HANGUL_BASE+
                     ((a-Hangul::JAMO_L_BASE)*Hangul::JAMO_V_COUNT+b)*
                     Hangul::JAMO_T_COUNT);
            } else {
                return U_SENTINEL;
            }
        } else if(isHangul(norm16)) {
            b-=Hangul::JAMO_T_BASE;
            if(Hangul::isHangulWithoutJamoT(a) && 0<b && b<Hangul::JAMO_T_COUNT) {  
                return a+b;
            } else {
                return U_SENTINEL;
            }
        } else {
            
            list=extraData+norm16;
            if(norm16>minYesNo) {  
                list+=  
                    1+  
                    (*list&MAPPING_LENGTH_MASK);  
            }
        }
    } else if(norm16<minMaybeYes || MIN_NORMAL_MAYBE_YES<=norm16) {
        return U_SENTINEL;
    } else {
        list=maybeYesCompositions+norm16-minMaybeYes;
    }
    if(b<0 || 0x10ffff<b) {  
        return U_SENTINEL;
    }
#if U_SIGNED_RIGHT_SHIFT_IS_ARITHMETIC
    return combine(list, b)>>1;
#else
    int32_t compositeAndFwd=combine(list, b);
    return compositeAndFwd>=0 ? compositeAndFwd>>1 : U_SENTINEL;
#endif
}




UBool
Normalizer2Impl::compose(const UChar *src, const UChar *limit,
                         UBool onlyContiguous,
                         UBool doCompose,
                         ReorderingBuffer &buffer,
                         UErrorCode &errorCode) const {
    











    const UChar *prevBoundary=src;
    UChar32 minNoMaybeCP=minCompNoMaybeCP;
    if(limit==NULL) {
        src=copyLowPrefixFromNulTerminated(src, minNoMaybeCP,
                                           doCompose ? &buffer : NULL,
                                           errorCode);
        if(U_FAILURE(errorCode)) {
            return FALSE;
        }
        if(prevBoundary<src) {
            
            prevBoundary=src-1;
        }
        limit=u_strchr(src, 0);
    }

    const UChar *prevSrc;
    UChar32 c=0;
    uint16_t norm16=0;

    
    uint8_t prevCC=0;

    for(;;) {
        
        for(prevSrc=src; src!=limit;) {
            if( (c=*src)<minNoMaybeCP ||
                isCompYesAndZeroCC(norm16=UTRIE2_GET16_FROM_U16_SINGLE_LEAD(normTrie, c))
            ) {
                ++src;
            } else if(!U16_IS_SURROGATE(c)) {
                break;
            } else {
                UChar c2;
                if(U16_IS_SURROGATE_LEAD(c)) {
                    if((src+1)!=limit && U16_IS_TRAIL(c2=src[1])) {
                        c=U16_GET_SUPPLEMENTARY(c, c2);
                    }
                } else  {
                    if(prevSrc<src && U16_IS_LEAD(c2=*(src-1))) {
                        --src;
                        c=U16_GET_SUPPLEMENTARY(c2, c);
                    }
                }
                if(isCompYesAndZeroCC(norm16=getNorm16(c))) {
                    src+=U16_LENGTH(c);
                } else {
                    break;
                }
            }
        }
        
        if(src!=prevSrc) {
            if(doCompose) {
                if(!buffer.appendZeroCC(prevSrc, src, errorCode)) {
                    break;
                }
            } else {
                prevCC=0;
            }
            if(src==limit) {
                break;
            }
            
            prevBoundary=src-1;
            if( U16_IS_TRAIL(*prevBoundary) && prevSrc<prevBoundary &&
                U16_IS_LEAD(*(prevBoundary-1))
            ) {
                --prevBoundary;
            }
            
            prevSrc=src;
        } else if(src==limit) {
            break;
        }

        src+=U16_LENGTH(c);
        






        if(isJamoVT(norm16) && prevBoundary!=prevSrc) {
            UChar prev=*(prevSrc-1);
            UBool needToDecompose=FALSE;
            if(c<Hangul::JAMO_T_BASE) {
                
                prev=(UChar)(prev-Hangul::JAMO_L_BASE);
                if(prev<Hangul::JAMO_L_COUNT) {
                    if(!doCompose) {
                        return FALSE;
                    }
                    UChar syllable=(UChar)
                        (Hangul::HANGUL_BASE+
                         (prev*Hangul::JAMO_V_COUNT+(c-Hangul::JAMO_V_BASE))*
                         Hangul::JAMO_T_COUNT);
                    UChar t;
                    if(src!=limit && (t=(UChar)(*src-Hangul::JAMO_T_BASE))<Hangul::JAMO_T_COUNT) {
                        ++src;
                        syllable+=t;  
                        prevBoundary=src;
                        buffer.setLastChar(syllable);
                        continue;
                    }
                    
                    
                    
                    
                    
                    
                    
                    
                    
                    needToDecompose=TRUE;
                }
            } else if(Hangul::isHangulWithoutJamoT(prev)) {
                
                
                if(!doCompose) {
                    return FALSE;
                }
                buffer.setLastChar((UChar)(prev+c-Hangul::JAMO_T_BASE));
                prevBoundary=src;
                continue;
            }
            if(!needToDecompose) {
                
                if(doCompose) {
                    if(!buffer.appendBMP((UChar)c, 0, errorCode)) {
                        break;
                    }
                } else {
                    prevCC=0;
                }
                continue;
            }
        }
        





















        if(norm16>=MIN_YES_YES_WITH_CC) {
            uint8_t cc=(uint8_t)norm16;  
            if( onlyContiguous &&  
                (doCompose ? buffer.getLastCC() : prevCC)==0 &&
                prevBoundary<prevSrc &&
                
                
                
                
                
                
                
                getTrailCCFromCompYesAndZeroCC(prevBoundary, prevSrc)>cc
            ) {
                
                if(!doCompose) {
                    return FALSE;
                }
            } else if(doCompose) {
                if(!buffer.append(c, cc, errorCode)) {
                    break;
                }
                continue;
            } else if(prevCC<=cc) {
                prevCC=cc;
                continue;
            } else {
                return FALSE;
            }
        } else if(!doCompose && !isMaybeOrNonZeroCC(norm16)) {
            return FALSE;
        }

        









        




        if(hasCompBoundaryBefore(c, norm16)) {
            prevBoundary=prevSrc;
        } else if(doCompose) {
            buffer.removeSuffix((int32_t)(prevSrc-prevBoundary));
        }

        
        
        src=(UChar *)findNextCompBoundary(src, limit);

        
        int32_t recomposeStartIndex=buffer.length();
        if(!decomposeShort(prevBoundary, src, buffer, errorCode)) {
            break;
        }
        recompose(buffer, recomposeStartIndex, onlyContiguous);
        if(!doCompose) {
            if(!buffer.equals(prevBoundary, src)) {
                return FALSE;
            }
            buffer.remove();
            prevCC=0;
        }

        
        prevBoundary=src;
    }
    return TRUE;
}




const UChar *
Normalizer2Impl::composeQuickCheck(const UChar *src, const UChar *limit,
                                   UBool onlyContiguous,
                                   UNormalizationCheckResult *pQCResult) const {
    



    const UChar *prevBoundary=src;
    UChar32 minNoMaybeCP=minCompNoMaybeCP;
    if(limit==NULL) {
        UErrorCode errorCode=U_ZERO_ERROR;
        src=copyLowPrefixFromNulTerminated(src, minNoMaybeCP, NULL, errorCode);
        if(prevBoundary<src) {
            
            prevBoundary=src-1;
        }
        limit=u_strchr(src, 0);
    }

    const UChar *prevSrc;
    UChar32 c=0;
    uint16_t norm16=0;
    uint8_t prevCC=0;

    for(;;) {
        
        for(prevSrc=src;;) {
            if(src==limit) {
                return src;
            }
            if( (c=*src)<minNoMaybeCP ||
                isCompYesAndZeroCC(norm16=UTRIE2_GET16_FROM_U16_SINGLE_LEAD(normTrie, c))
            ) {
                ++src;
            } else if(!U16_IS_SURROGATE(c)) {
                break;
            } else {
                UChar c2;
                if(U16_IS_SURROGATE_LEAD(c)) {
                    if((src+1)!=limit && U16_IS_TRAIL(c2=src[1])) {
                        c=U16_GET_SUPPLEMENTARY(c, c2);
                    }
                } else  {
                    if(prevSrc<src && U16_IS_LEAD(c2=*(src-1))) {
                        --src;
                        c=U16_GET_SUPPLEMENTARY(c2, c);
                    }
                }
                if(isCompYesAndZeroCC(norm16=getNorm16(c))) {
                    src+=U16_LENGTH(c);
                } else {
                    break;
                }
            }
        }
        if(src!=prevSrc) {
            
            prevBoundary=src-1;
            if( U16_IS_TRAIL(*prevBoundary) && prevSrc<prevBoundary &&
                U16_IS_LEAD(*(prevBoundary-1))
            ) {
                --prevBoundary;
            }
            prevCC=0;
            
            prevSrc=src;
        }

        src+=U16_LENGTH(c);
        




        if(isMaybeOrNonZeroCC(norm16)) {
            uint8_t cc=getCCFromYesOrMaybe(norm16);
            if( onlyContiguous &&  
                cc!=0 &&
                prevCC==0 &&
                prevBoundary<prevSrc &&
                
                
                
                
                
                
                
                getTrailCCFromCompYesAndZeroCC(prevBoundary, prevSrc)>cc
            ) {
                
            } else if(prevCC<=cc || cc==0) {
                prevCC=cc;
                if(norm16<MIN_YES_YES_WITH_CC) {
                    if(pQCResult!=NULL) {
                        *pQCResult=UNORM_MAYBE;
                    } else {
                        return prevBoundary;
                    }
                }
                continue;
            }
        }
        if(pQCResult!=NULL) {
            *pQCResult=UNORM_NO;
        }
        return prevBoundary;
    }
}

void Normalizer2Impl::composeAndAppend(const UChar *src, const UChar *limit,
                                       UBool doCompose,
                                       UBool onlyContiguous,
                                       UnicodeString &safeMiddle,
                                       ReorderingBuffer &buffer,
                                       UErrorCode &errorCode) const {
    if(!buffer.isEmpty()) {
        const UChar *firstStarterInSrc=findNextCompBoundary(src, limit);
        if(src!=firstStarterInSrc) {
            const UChar *lastStarterInDest=findPreviousCompBoundary(buffer.getStart(),
                                                                    buffer.getLimit());
            int32_t destSuffixLength=(int32_t)(buffer.getLimit()-lastStarterInDest);
            UnicodeString middle(lastStarterInDest, destSuffixLength);
            buffer.removeSuffix(destSuffixLength);
            safeMiddle=middle;
            middle.append(src, (int32_t)(firstStarterInSrc-src));
            const UChar *middleStart=middle.getBuffer();
            compose(middleStart, middleStart+middle.length(), onlyContiguous,
                    TRUE, buffer, errorCode);
            if(U_FAILURE(errorCode)) {
                return;
            }
            src=firstStarterInSrc;
        }
    }
    if(doCompose) {
        compose(src, limit, onlyContiguous, TRUE, buffer, errorCode);
    } else {
        if(limit==NULL) {  
            limit=u_strchr(src, 0);
        }
        buffer.appendZeroCC(src, limit, errorCode);
    }
}








UBool Normalizer2Impl::hasCompBoundaryBefore(UChar32 c, uint16_t norm16) const {
    for(;;) {
        if(isCompYesAndZeroCC(norm16)) {
            return TRUE;
        } else if(isMaybeOrNonZeroCC(norm16)) {
            return FALSE;
        } else if(isDecompNoAlgorithmic(norm16)) {
            c=mapAlgorithmic(c, norm16);
            norm16=getNorm16(c);
        } else {
            
            const uint16_t *mapping=getMapping(norm16);
            uint16_t firstUnit=*mapping;
            if((firstUnit&MAPPING_LENGTH_MASK)==0) {
                return FALSE;
            }
            if((firstUnit&MAPPING_HAS_CCC_LCCC_WORD) && (*(mapping-1)&0xff00)) {
                return FALSE;  
            }
            int32_t i=1;  
            UChar32 c;
            U16_NEXT_UNSAFE(mapping, i, c);
            return isCompYesAndZeroCC(getNorm16(c));
        }
    }
}

UBool Normalizer2Impl::hasCompBoundaryAfter(UChar32 c, UBool onlyContiguous, UBool testInert) const {
    for(;;) {
        uint16_t norm16=getNorm16(c);
        if(isInert(norm16)) {
            return TRUE;
        } else if(norm16<=minYesNo) {
            
            
            
            return isHangul(norm16) && !Hangul::isHangulWithoutJamoT((UChar)c);
        } else if(norm16>= (testInert ? minNoNo : minMaybeYes)) {
            return FALSE;
        } else if(isDecompNoAlgorithmic(norm16)) {
            c=mapAlgorithmic(c, norm16);
        } else {
            
            
            
            const uint16_t *mapping=getMapping(norm16);
            uint16_t firstUnit=*mapping;
            
            
            
            
            
            
            return
                (firstUnit&MAPPING_NO_COMP_BOUNDARY_AFTER)==0 &&
                (!onlyContiguous || firstUnit<=0x1ff);
        }
    }
}

const UChar *Normalizer2Impl::findPreviousCompBoundary(const UChar *start, const UChar *p) const {
    BackwardUTrie2StringIterator iter(normTrie, start, p);
    uint16_t norm16;
    do {
        norm16=iter.previous16();
    } while(!hasCompBoundaryBefore(iter.codePoint, norm16));
    
    
    return iter.codePointStart;
}

const UChar *Normalizer2Impl::findNextCompBoundary(const UChar *p, const UChar *limit) const {
    ForwardUTrie2StringIterator iter(normTrie, p, limit);
    uint16_t norm16;
    do {
        norm16=iter.next16();
    } while(!hasCompBoundaryBefore(iter.codePoint, norm16));
    return iter.codePointStart;
}










uint16_t Normalizer2Impl::getFCD16FromNormData(UChar32 c) const {
    
    for(;;) {
        uint16_t norm16=getNorm16(c);
        if(norm16<=minYesNo) {
            
            return 0;
        } else if(norm16>=MIN_NORMAL_MAYBE_YES) {
            
            norm16&=0xff;
            return norm16|(norm16<<8);
        } else if(norm16>=minMaybeYes) {
            return 0;
        } else if(isDecompNoAlgorithmic(norm16)) {
            c=mapAlgorithmic(c, norm16);
        } else {
            
            const uint16_t *mapping=getMapping(norm16);
            uint16_t firstUnit=*mapping;
            if((firstUnit&MAPPING_LENGTH_MASK)==0) {
                
                
                
                return 0x1ff;
            } else {
                norm16=firstUnit>>8;  
                if(firstUnit&MAPPING_HAS_CCC_LCCC_WORD) {
                    norm16|=*(mapping-1)&0xff00;  
                }
                return norm16;
            }
        }
    }
}




const UChar *
Normalizer2Impl::makeFCD(const UChar *src, const UChar *limit,
                         ReorderingBuffer *buffer,
                         UErrorCode &errorCode) const {
    
    
    const UChar *prevBoundary=src;
    int32_t prevFCD16=0;
    if(limit==NULL) {
        src=copyLowPrefixFromNulTerminated(src, MIN_CCC_LCCC_CP, buffer, errorCode);
        if(U_FAILURE(errorCode)) {
            return src;
        }
        if(prevBoundary<src) {
            prevBoundary=src;
            
            
            prevFCD16=getFCD16(*(src-1));
            if(prevFCD16>1) {
                --prevBoundary;
            }
        }
        limit=u_strchr(src, 0);
    }

    
    
    
    
    

    const UChar *prevSrc;
    UChar32 c=0;
    uint16_t fcd16=0;

    for(;;) {
        
        for(prevSrc=src; src!=limit;) {
            if((c=*src)<MIN_CCC_LCCC_CP) {
                prevFCD16=~c;
                ++src;
            } else if(!singleLeadMightHaveNonZeroFCD16(c)) {
                prevFCD16=0;
                ++src;
            } else {
                if(U16_IS_SURROGATE(c)) {
                    UChar c2;
                    if(U16_IS_SURROGATE_LEAD(c)) {
                        if((src+1)!=limit && U16_IS_TRAIL(c2=src[1])) {
                            c=U16_GET_SUPPLEMENTARY(c, c2);
                        }
                    } else  {
                        if(prevSrc<src && U16_IS_LEAD(c2=*(src-1))) {
                            --src;
                            c=U16_GET_SUPPLEMENTARY(c2, c);
                        }
                    }
                }
                if((fcd16=getFCD16FromNormData(c))<=0xff) {
                    prevFCD16=fcd16;
                    src+=U16_LENGTH(c);
                } else {
                    break;
                }
            }
        }
        
        if(src!=prevSrc) {
            if(buffer!=NULL && !buffer->appendZeroCC(prevSrc, src, errorCode)) {
                break;
            }
            if(src==limit) {
                break;
            }
            prevBoundary=src;
            
            if(prevFCD16<0) {
                
                UChar32 prev=~prevFCD16;
                prevFCD16= prev<0x180 ? tccc180[prev] : getFCD16FromNormData(prev);
                if(prevFCD16>1) {
                    --prevBoundary;
                }
            } else {
                const UChar *p=src-1;
                if(U16_IS_TRAIL(*p) && prevSrc<p && U16_IS_LEAD(*(p-1))) {
                    --p;
                    
                    
                    prevFCD16=getFCD16FromNormData(U16_GET_SUPPLEMENTARY(p[0], p[1]));
                    
                }
                if(prevFCD16>1) {
                    prevBoundary=p;
                }
            }
            
            prevSrc=src;
        } else if(src==limit) {
            break;
        }

        src+=U16_LENGTH(c);
        
        
        if((prevFCD16&0xff)<=(fcd16>>8)) {
            
            if((fcd16&0xff)<=1) {
                prevBoundary=src;
            }
            if(buffer!=NULL && !buffer->appendZeroCC(c, errorCode)) {
                break;
            }
            prevFCD16=fcd16;
            continue;
        } else if(buffer==NULL) {
            return prevBoundary;  
        } else {
            




            buffer->removeSuffix((int32_t)(prevSrc-prevBoundary));
            



            src=findNextFCDBoundary(src, limit);
            



            if(!decomposeShort(prevBoundary, src, *buffer, errorCode)) {
                break;
            }
            prevBoundary=src;
            prevFCD16=0;
        }
    }
    return src;
}

void Normalizer2Impl::makeFCDAndAppend(const UChar *src, const UChar *limit,
                                       UBool doMakeFCD,
                                       UnicodeString &safeMiddle,
                                       ReorderingBuffer &buffer,
                                       UErrorCode &errorCode) const {
    if(!buffer.isEmpty()) {
        const UChar *firstBoundaryInSrc=findNextFCDBoundary(src, limit);
        if(src!=firstBoundaryInSrc) {
            const UChar *lastBoundaryInDest=findPreviousFCDBoundary(buffer.getStart(),
                                                                    buffer.getLimit());
            int32_t destSuffixLength=(int32_t)(buffer.getLimit()-lastBoundaryInDest);
            UnicodeString middle(lastBoundaryInDest, destSuffixLength);
            buffer.removeSuffix(destSuffixLength);
            safeMiddle=middle;
            middle.append(src, (int32_t)(firstBoundaryInSrc-src));
            const UChar *middleStart=middle.getBuffer();
            makeFCD(middleStart, middleStart+middle.length(), &buffer, errorCode);
            if(U_FAILURE(errorCode)) {
                return;
            }
            src=firstBoundaryInSrc;
        }
    }
    if(doMakeFCD) {
        makeFCD(src, limit, &buffer, errorCode);
    } else {
        if(limit==NULL) {  
            limit=u_strchr(src, 0);
        }
        buffer.appendZeroCC(src, limit, errorCode);
    }
}

const UChar *Normalizer2Impl::findPreviousFCDBoundary(const UChar *start, const UChar *p) const {
    while(start<p && previousFCD16(start, p)>0xff) {}
    return p;
}

const UChar *Normalizer2Impl::findNextFCDBoundary(const UChar *p, const UChar *limit) const {
    while(p<limit) {
        const UChar *codePointStart=p;
        if(nextFCD16(p, limit)<=0xff) {
            return codePointStart;
        }
    }
    return p;
}



CanonIterData::CanonIterData(UErrorCode &errorCode) :
        trie(utrie2_open(0, 0, &errorCode)),
        canonStartSets(uprv_deleteUObject, NULL, errorCode) {}

CanonIterData::~CanonIterData() {
    utrie2_close(trie);
}

void CanonIterData::addToStartSet(UChar32 origin, UChar32 decompLead, UErrorCode &errorCode) {
    uint32_t canonValue=utrie2_get32(trie, decompLead);
    if((canonValue&(CANON_HAS_SET|CANON_VALUE_MASK))==0 && origin!=0) {
        
        
        utrie2_set32(trie, decompLead, canonValue|origin, &errorCode);
    } else {
        
        UnicodeSet *set;
        if((canonValue&CANON_HAS_SET)==0) {
            set=new UnicodeSet;
            if(set==NULL) {
                errorCode=U_MEMORY_ALLOCATION_ERROR;
                return;
            }
            UChar32 firstOrigin=(UChar32)(canonValue&CANON_VALUE_MASK);
            canonValue=(canonValue&~CANON_VALUE_MASK)|CANON_HAS_SET|(uint32_t)canonStartSets.size();
            utrie2_set32(trie, decompLead, canonValue, &errorCode);
            canonStartSets.addElement(set, errorCode);
            if(firstOrigin!=0) {
                set->add(firstOrigin);
            }
        } else {
            set=(UnicodeSet *)canonStartSets[(int32_t)(canonValue&CANON_VALUE_MASK)];
        }
        set->add(origin);
    }
}

class CanonIterDataSingleton {
public:
    CanonIterDataSingleton(SimpleSingleton &s, Normalizer2Impl &ni, UErrorCode &ec) :
        singleton(s), impl(ni), errorCode(ec) {}
    CanonIterData *getInstance(UErrorCode &errorCode) {
        void *duplicate;
        CanonIterData *instance=
            (CanonIterData *)singleton.getInstance(createInstance, this, duplicate, errorCode);
        delete (CanonIterData *)duplicate;
        return instance;
    }
    static void *createInstance(const void *context, UErrorCode &errorCode);
    UBool rangeHandler(UChar32 start, UChar32 end, uint32_t value) {
        if(value!=0) {
            impl.makeCanonIterDataFromNorm16(start, end, (uint16_t)value, *newData, errorCode);
        }
        return U_SUCCESS(errorCode);
    }

private:
    SimpleSingleton &singleton;
    Normalizer2Impl &impl;
    CanonIterData *newData;
    UErrorCode &errorCode;
};

U_CDECL_BEGIN


static UBool U_CALLCONV
enumCIDRangeHandler(const void *context, UChar32 start, UChar32 end, uint32_t value) {
    return ((CanonIterDataSingleton *)context)->rangeHandler(start, end, value);
}

U_CDECL_END

void *CanonIterDataSingleton::createInstance(const void *context, UErrorCode &errorCode) {
    CanonIterDataSingleton *me=(CanonIterDataSingleton *)context;
    me->newData=new CanonIterData(errorCode);
    if(me->newData==NULL) {
        errorCode=U_MEMORY_ALLOCATION_ERROR;
        return NULL;
    }
    if(U_SUCCESS(errorCode)) {
        utrie2_enum(me->impl.getNormTrie(), NULL, enumCIDRangeHandler, me);
        utrie2_freeze(me->newData->trie, UTRIE2_32_VALUE_BITS, &errorCode);
        if(U_SUCCESS(errorCode)) {
            return me->newData;
        }
    }
    delete me->newData;
    return NULL;
}

void Normalizer2Impl::makeCanonIterDataFromNorm16(UChar32 start, UChar32 end, uint16_t norm16,
                                                  CanonIterData &newData,
                                                  UErrorCode &errorCode) const {
    if(norm16==0 || (minYesNo<=norm16 && norm16<minNoNo)) {
        
        
        
        
        
        
        return;
    }
    for(UChar32 c=start; c<=end; ++c) {
        uint32_t oldValue=utrie2_get32(newData.trie, c);
        uint32_t newValue=oldValue;
        if(norm16>=minMaybeYes) {
            
            newValue|=CANON_NOT_SEGMENT_STARTER;
            if(norm16<MIN_NORMAL_MAYBE_YES) {
                newValue|=CANON_HAS_COMPOSITIONS;
            }
        } else if(norm16<minYesNo) {
            newValue|=CANON_HAS_COMPOSITIONS;
        } else {
            
            UChar32 c2=c;
            uint16_t norm16_2=norm16;
            while(limitNoNo<=norm16_2 && norm16_2<minMaybeYes) {
                c2=mapAlgorithmic(c2, norm16_2);
                norm16_2=getNorm16(c2);
            }
            if(minYesNo<=norm16_2 && norm16_2<limitNoNo) {
                
                const uint16_t *mapping=getMapping(norm16_2);
                uint16_t firstUnit=*mapping;
                int32_t length=firstUnit&MAPPING_LENGTH_MASK;
                if((firstUnit&MAPPING_HAS_CCC_LCCC_WORD)!=0) {
                    if(c==c2 && (*(mapping-1)&0xff)!=0) {
                        newValue|=CANON_NOT_SEGMENT_STARTER;  
                    }
                }
                
                if(length!=0) {
                    ++mapping;  
                    
                    int32_t i=0;
                    U16_NEXT_UNSAFE(mapping, i, c2);
                    newData.addToStartSet(c, c2, errorCode);
                    
                    
                    
                    if(norm16_2>=minNoNo) {
                        while(i<length) {
                            U16_NEXT_UNSAFE(mapping, i, c2);
                            uint32_t c2Value=utrie2_get32(newData.trie, c2);
                            if((c2Value&CANON_NOT_SEGMENT_STARTER)==0) {
                                utrie2_set32(newData.trie, c2, c2Value|CANON_NOT_SEGMENT_STARTER,
                                             &errorCode);
                            }
                        }
                    }
                }
            } else {
                
                newData.addToStartSet(c, c2, errorCode);
            }
        }
        if(newValue!=oldValue) {
            utrie2_set32(newData.trie, c, newValue, &errorCode);
        }
    }
}

UBool Normalizer2Impl::ensureCanonIterData(UErrorCode &errorCode) const {
    
    Normalizer2Impl *me=const_cast<Normalizer2Impl *>(this);
    CanonIterDataSingleton(me->canonIterDataSingleton, *me, errorCode).getInstance(errorCode);
    return U_SUCCESS(errorCode);
}

int32_t Normalizer2Impl::getCanonValue(UChar32 c) const {
    return (int32_t)utrie2_get32(((CanonIterData *)canonIterDataSingleton.fInstance)->trie, c);
}

const UnicodeSet &Normalizer2Impl::getCanonStartSet(int32_t n) const {
    return *(const UnicodeSet *)(
        ((CanonIterData *)canonIterDataSingleton.fInstance)->canonStartSets[n]);
}

UBool Normalizer2Impl::isCanonSegmentStarter(UChar32 c) const {
    return getCanonValue(c)>=0;
}

UBool Normalizer2Impl::getCanonStartSet(UChar32 c, UnicodeSet &set) const {
    int32_t canonValue=getCanonValue(c)&~CANON_NOT_SEGMENT_STARTER;
    if(canonValue==0) {
        return FALSE;
    }
    set.clear();
    int32_t value=canonValue&CANON_VALUE_MASK;
    if((canonValue&CANON_HAS_SET)!=0) {
        set.addAll(getCanonStartSet(value));
    } else if(value!=0) {
        set.add(value);
    }
    if((canonValue&CANON_HAS_COMPOSITIONS)!=0) {
        uint16_t norm16=getNorm16(c);
        if(norm16==JAMO_L) {
            UChar32 syllable=
                (UChar32)(Hangul::HANGUL_BASE+(c-Hangul::JAMO_L_BASE)*Hangul::JAMO_VT_COUNT);
            set.add(syllable, syllable+Hangul::JAMO_VT_COUNT-1);
        } else {
            addComposites(getCompositionsList(norm16), set);
        }
    }
    return TRUE;
}

U_NAMESPACE_END



U_NAMESPACE_USE

U_CAPI int32_t U_EXPORT2
unorm2_swap(const UDataSwapper *ds,
            const void *inData, int32_t length, void *outData,
            UErrorCode *pErrorCode) {
    const UDataInfo *pInfo;
    int32_t headerSize;

    const uint8_t *inBytes;
    uint8_t *outBytes;

    const int32_t *inIndexes;
    int32_t indexes[Normalizer2Impl::IX_MIN_MAYBE_YES+1];

    int32_t i, offset, nextOffset, size;

    
    headerSize=udata_swapDataHeader(ds, inData, length, outData, pErrorCode);
    if(pErrorCode==NULL || U_FAILURE(*pErrorCode)) {
        return 0;
    }

    
    pInfo=(const UDataInfo *)((const char *)inData+4);
    if(!(
        pInfo->dataFormat[0]==0x4e &&   
        pInfo->dataFormat[1]==0x72 &&
        pInfo->dataFormat[2]==0x6d &&
        pInfo->dataFormat[3]==0x32 &&
        (pInfo->formatVersion[0]==1 || pInfo->formatVersion[0]==2)
    )) {
        udata_printError(ds, "unorm2_swap(): data format %02x.%02x.%02x.%02x (format version %02x) is not recognized as Normalizer2 data\n",
                         pInfo->dataFormat[0], pInfo->dataFormat[1],
                         pInfo->dataFormat[2], pInfo->dataFormat[3],
                         pInfo->formatVersion[0]);
        *pErrorCode=U_UNSUPPORTED_ERROR;
        return 0;
    }

    inBytes=(const uint8_t *)inData+headerSize;
    outBytes=(uint8_t *)outData+headerSize;

    inIndexes=(const int32_t *)inBytes;

    if(length>=0) {
        length-=headerSize;
        if(length<(int32_t)sizeof(indexes)) {
            udata_printError(ds, "unorm2_swap(): too few bytes (%d after header) for Normalizer2 data\n",
                             length);
            *pErrorCode=U_INDEX_OUTOFBOUNDS_ERROR;
            return 0;
        }
    }

    
    for(i=0; i<=Normalizer2Impl::IX_MIN_MAYBE_YES; ++i) {
        indexes[i]=udata_readInt32(ds, inIndexes[i]);
    }

    
    size=indexes[Normalizer2Impl::IX_TOTAL_SIZE];

    if(length>=0) {
        if(length<size) {
            udata_printError(ds, "unorm2_swap(): too few bytes (%d after header) for all of Normalizer2 data\n",
                             length);
            *pErrorCode=U_INDEX_OUTOFBOUNDS_ERROR;
            return 0;
        }

        
        if(inBytes!=outBytes) {
            uprv_memcpy(outBytes, inBytes, size);
        }

        offset=0;

        
        nextOffset=indexes[Normalizer2Impl::IX_NORM_TRIE_OFFSET];
        ds->swapArray32(ds, inBytes, nextOffset-offset, outBytes, pErrorCode);
        offset=nextOffset;

        
        nextOffset=indexes[Normalizer2Impl::IX_EXTRA_DATA_OFFSET];
        utrie2_swap(ds, inBytes+offset, nextOffset-offset, outBytes+offset, pErrorCode);
        offset=nextOffset;

        
        nextOffset=indexes[Normalizer2Impl::IX_SMALL_FCD_OFFSET];
        ds->swapArray16(ds, inBytes+offset, nextOffset-offset, outBytes+offset, pErrorCode);
        offset=nextOffset;

        
        nextOffset=indexes[Normalizer2Impl::IX_SMALL_FCD_OFFSET+1];
        offset=nextOffset;

        U_ASSERT(offset==size);
    }

    return headerSize+size;
}

#endif  
