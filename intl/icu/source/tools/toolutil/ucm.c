























#include "unicode/utypes.h"
#include "unicode/ustring.h"
#include "cstring.h"
#include "cmemory.h"
#include "filestrm.h"
#include "uarrsort.h"
#include "ucnvmbcs.h"
#include "ucnv_bld.h"
#include "ucnv_ext.h"
#include "uparse.h"
#include "ucm.h"
#include <stdio.h>

#if !UCONFIG_NO_CONVERSION



static void
printMapping(UCMapping *m, UChar32 *codePoints, uint8_t *bytes, FILE *f) {
    int32_t j;

    for(j=0; j<m->uLen; ++j) {
        fprintf(f, "<U%04lX>", (long)codePoints[j]);
    }

    fputc(' ', f);

    for(j=0; j<m->bLen; ++j) {
        fprintf(f, "\\x%02X", bytes[j]);
    }

    if(m->f>=0) {
        fprintf(f, " |%u\n", m->f);
    } else {
        fputs("\n", f);
    }
}

U_CAPI void U_EXPORT2
ucm_printMapping(UCMTable *table, UCMapping *m, FILE *f) {
    printMapping(m, UCM_GET_CODE_POINTS(table, m), UCM_GET_BYTES(table, m), f);
}

U_CAPI void U_EXPORT2
ucm_printTable(UCMTable *table, FILE *f, UBool byUnicode) {
    UCMapping *m;
    int32_t i, length;

    m=table->mappings;
    length=table->mappingsLength;
    if(byUnicode) {
        for(i=0; i<length; ++m, ++i) {
            ucm_printMapping(table, m, f);
        }
    } else {
        const int32_t *map=table->reverseMap;
        for(i=0; i<length; ++i) {
            ucm_printMapping(table, m+map[i], f);
        }
    }
}



static int32_t
compareUnicode(UCMTable *lTable, const UCMapping *l,
               UCMTable *rTable, const UCMapping *r) {
    const UChar32 *lu, *ru;
    int32_t result, i, length;

    if(l->uLen==1 && r->uLen==1) {
        
        return l->u-r->u;
    }

    
    lu=UCM_GET_CODE_POINTS(lTable, l);
    ru=UCM_GET_CODE_POINTS(rTable, r);

    
    if(l->uLen<=r->uLen) {
        length=l->uLen;
    } else {
        length=r->uLen;
    }

    
    for(i=0; i<length; ++i) {
        result=lu[i]-ru[i];
        if(result!=0) {
            return result;
        }
    }

    
    return l->uLen-r->uLen;
}

static int32_t
compareBytes(UCMTable *lTable, const UCMapping *l,
             UCMTable *rTable, const UCMapping *r,
             UBool lexical) {
    const uint8_t *lb, *rb;
    int32_t result, i, length;

    







    if(lexical) {
        
        if(l->bLen<=r->bLen) {
            length=l->bLen;
        } else {
            length=r->bLen;
        }
    } else {
        
        result=l->bLen-r->bLen;
        if(result!=0) {
            return result;
        } else {
            length=l->bLen;
        }
    }

    
    lb=UCM_GET_BYTES(lTable, l);
    rb=UCM_GET_BYTES(rTable, r);

    
    for(i=0; i<length; ++i) {
        result=lb[i]-rb[i];
        if(result!=0) {
            return result;
        }
    }

    
    return l->bLen-r->bLen;
}


static int32_t
compareMappings(UCMTable *lTable, const UCMapping *l,
                UCMTable *rTable, const UCMapping *r,
                UBool uFirst) {
    int32_t result;

    
    if(uFirst) {
        
        result=compareUnicode(lTable, l, rTable, r);
        if(result==0) {
            result=compareBytes(lTable, l, rTable, r, FALSE); 
        }
    } else {
        
        result=compareBytes(lTable, l, rTable, r, TRUE); 
        if(result==0) {
            result=compareUnicode(lTable, l, rTable, r);
        }
    }

    if(result!=0) {
        return result;
    }

    
    return l->f-r->f;
}


static int32_t
compareMappingsUnicodeFirst(const void *context, const void *left, const void *right) {
    return compareMappings(
        (UCMTable *)context, (const UCMapping *)left,
        (UCMTable *)context, (const UCMapping *)right, TRUE);
}


static int32_t
compareMappingsBytesFirst(const void *context, const void *left, const void *right) {
    UCMTable *table=(UCMTable *)context;
    int32_t l=*(const int32_t *)left, r=*(const int32_t *)right;
    return compareMappings(
        table, table->mappings+l,
        table, table->mappings+r, FALSE);
}

U_CAPI void U_EXPORT2
ucm_sortTable(UCMTable *t) {
    UErrorCode errorCode;
    int32_t i;

    if(t->isSorted) {
        return;
    }

    errorCode=U_ZERO_ERROR;

    
    uprv_sortArray(t->mappings, t->mappingsLength, sizeof(UCMapping),
                   compareMappingsUnicodeFirst, t,
                   FALSE, &errorCode);

    
    if(t->reverseMap==NULL) {
        





        t->reverseMap=(int32_t *)uprv_malloc(t->mappingsCapacity*sizeof(int32_t));
        if(t->reverseMap==NULL) {
            fprintf(stderr, "ucm error: unable to allocate reverseMap\n");
            exit(U_MEMORY_ALLOCATION_ERROR);
        }
    }
    for(i=0; i<t->mappingsLength; ++i) {
        t->reverseMap[i]=i;
    }

    
    uprv_sortArray(t->reverseMap, t->mappingsLength, sizeof(int32_t),
                   compareMappingsBytesFirst, t,
                   FALSE, &errorCode);

    if(U_FAILURE(errorCode)) {
        fprintf(stderr, "ucm error: sortTable()/uprv_sortArray() fails - %s\n",
                u_errorName(errorCode));
        exit(errorCode);
    }

    t->isSorted=TRUE;
}





U_CAPI void U_EXPORT2
ucm_moveMappings(UCMTable *base, UCMTable *ext) {
    UCMapping *mb, *mbLimit;
    int8_t flag;

    mb=base->mappings;
    mbLimit=mb+base->mappingsLength;

    while(mb<mbLimit) {
        flag=mb->moveFlag;
        if(flag!=0) {
            
            mb->moveFlag=0;

            if(ext!=NULL && (flag&UCM_MOVE_TO_EXT)) {
                
                ucm_addMapping(ext, mb, UCM_GET_CODE_POINTS(base, mb), UCM_GET_BYTES(base, mb));
            }

            
            if(mb<(mbLimit-1)) {
                uprv_memcpy(mb, mbLimit-1, sizeof(UCMapping));
            }
            --mbLimit;
            --base->mappingsLength;
            base->isSorted=FALSE;
        } else {
            ++mb;
        }
    }
}

enum {
    NEEDS_MOVE=1,
    HAS_ERRORS=2
};

static uint8_t
checkBaseExtUnicode(UCMStates *baseStates, UCMTable *base, UCMTable *ext,
                    UBool moveToExt, UBool intersectBase) {
    UCMapping *mb, *me, *mbLimit, *meLimit;
    int32_t cmp;
    uint8_t result;

    mb=base->mappings;
    mbLimit=mb+base->mappingsLength;

    me=ext->mappings;
    meLimit=me+ext->mappingsLength;

    result=0;

    for(;;) {
        
        for(;;) {
            if(mb==mbLimit) {
                return result;
            }

            if(0<=mb->f && mb->f<=2) {
                break;
            }

            ++mb;
        }

        for(;;) {
            if(me==meLimit) {
                return result;
            }

            if(0<=me->f && me->f<=2) {
                break;
            }

            ++me;
        }

        
        cmp=compareUnicode(base, mb, ext, me);
        if(cmp<0) {
            if(intersectBase && (intersectBase!=2 || mb->bLen>1)) {
                





                mb->moveFlag|=UCM_MOVE_TO_EXT;
                result|=NEEDS_MOVE;

            
            } else if( mb->uLen<me->uLen &&
                0==uprv_memcmp(UCM_GET_CODE_POINTS(base, mb), UCM_GET_CODE_POINTS(ext, me), 4*mb->uLen)
            ) {
                if(moveToExt) {
                    
                    mb->moveFlag|=UCM_MOVE_TO_EXT;
                    result|=NEEDS_MOVE;
                } else {
                    fprintf(stderr,
                            "ucm error: the base table contains a mapping whose input sequence\n"
                            "           is a prefix of the input sequence of an extension mapping\n");
                    ucm_printMapping(base, mb, stderr);
                    ucm_printMapping(ext, me, stderr);
                    result|=HAS_ERRORS;
                }
            }

            ++mb;
        } else if(cmp==0) {
            



            if( mb->f==me->f && mb->bLen==me->bLen &&
                0==uprv_memcmp(UCM_GET_BYTES(base, mb), UCM_GET_BYTES(ext, me), mb->bLen)
            ) {
                me->moveFlag|=UCM_REMOVE_MAPPING;
                result|=NEEDS_MOVE;
            } else if(intersectBase) {
                
                mb->moveFlag|=UCM_MOVE_TO_EXT;
                result|=NEEDS_MOVE;
            } else {
                fprintf(stderr,
                        "ucm error: the base table contains a mapping whose input sequence\n"
                        "           is the same as the input sequence of an extension mapping\n"
                        "           but it maps differently\n");
                ucm_printMapping(base, mb, stderr);
                ucm_printMapping(ext, me, stderr);
                result|=HAS_ERRORS;
            }

            ++mb;
        } else  {
            ++me;
        }
    }
}

static uint8_t
checkBaseExtBytes(UCMStates *baseStates, UCMTable *base, UCMTable *ext,
                  UBool moveToExt, UBool intersectBase) {
    UCMapping *mb, *me;
    int32_t *baseMap, *extMap;
    int32_t b, e, bLimit, eLimit, cmp;
    uint8_t result;
    UBool isSISO;

    baseMap=base->reverseMap;
    extMap=ext->reverseMap;

    b=e=0;
    bLimit=base->mappingsLength;
    eLimit=ext->mappingsLength;

    result=0;

    isSISO=(UBool)(baseStates->outputType==MBCS_OUTPUT_2_SISO);

    for(;;) {
        
        for(;; ++b) {
            if(b==bLimit) {
                return result;
            }
            mb=base->mappings+baseMap[b];

            if(intersectBase==2 && mb->bLen==1) {
                



                continue;
            }

            if(mb->f==0 || mb->f==3) {
                break;
            }
        }

        for(;;) {
            if(e==eLimit) {
                return result;
            }
            me=ext->mappings+extMap[e];

            if(me->f==0 || me->f==3) {
                break;
            }

            ++e;
        }

        
        cmp=compareBytes(base, mb, ext, me, TRUE);
        if(cmp<0) {
            if(intersectBase) {
                
                mb->moveFlag|=UCM_MOVE_TO_EXT;
                result|=NEEDS_MOVE;

            




            } else if( mb->bLen<me->bLen &&
                (!isSISO || mb->bLen>1) &&
                0==uprv_memcmp(UCM_GET_BYTES(base, mb), UCM_GET_BYTES(ext, me), mb->bLen)
            ) {
                if(moveToExt) {
                    
                    mb->moveFlag|=UCM_MOVE_TO_EXT;
                    result|=NEEDS_MOVE;
                } else {
                    fprintf(stderr,
                            "ucm error: the base table contains a mapping whose input sequence\n"
                            "           is a prefix of the input sequence of an extension mapping\n");
                    ucm_printMapping(base, mb, stderr);
                    ucm_printMapping(ext, me, stderr);
                    result|=HAS_ERRORS;
                }
            }

            ++b;
        } else if(cmp==0) {
            



            if( mb->f==me->f && mb->uLen==me->uLen &&
                0==uprv_memcmp(UCM_GET_CODE_POINTS(base, mb), UCM_GET_CODE_POINTS(ext, me), 4*mb->uLen)
            ) {
                me->moveFlag|=UCM_REMOVE_MAPPING;
                result|=NEEDS_MOVE;
            } else if(intersectBase) {
                
                mb->moveFlag|=UCM_MOVE_TO_EXT;
                result|=NEEDS_MOVE;
            } else {
                fprintf(stderr,
                        "ucm error: the base table contains a mapping whose input sequence\n"
                        "           is the same as the input sequence of an extension mapping\n"
                        "           but it maps differently\n");
                ucm_printMapping(base, mb, stderr);
                ucm_printMapping(ext, me, stderr);
                result|=HAS_ERRORS;
            }

            ++b;
        } else  {
            ++e;
        }
    }
}

U_CAPI UBool U_EXPORT2
ucm_checkValidity(UCMTable *table, UCMStates *baseStates) {
    UCMapping *m, *mLimit;
    int32_t count;
    UBool isOK;

    m=table->mappings;
    mLimit=m+table->mappingsLength;
    isOK=TRUE;

    while(m<mLimit) {
        count=ucm_countChars(baseStates, UCM_GET_BYTES(table, m), m->bLen);
        if(count<1) {
            ucm_printMapping(table, m, stderr);
            isOK=FALSE;
        }
        ++m;
    }

    return isOK;
}

U_CAPI UBool U_EXPORT2
ucm_checkBaseExt(UCMStates *baseStates,
                 UCMTable *base, UCMTable *ext, UCMTable *moveTarget,
                 UBool intersectBase) {
    uint8_t result;

    
    if(base->flagsType&UCM_FLAGS_IMPLICIT) {
        fprintf(stderr, "ucm error: the base table contains mappings without precision flags\n");
        return FALSE;
    }
    if(ext->flagsType&UCM_FLAGS_IMPLICIT) {
        fprintf(stderr, "ucm error: extension table contains mappings without precision flags\n");
        return FALSE;
    }

    
    ucm_sortTable(base);
    ucm_sortTable(ext);

    
    result=
        checkBaseExtUnicode(baseStates, base, ext, (UBool)(moveTarget!=NULL), intersectBase)|
        checkBaseExtBytes(baseStates, base, ext, (UBool)(moveTarget!=NULL), intersectBase);

    if(result&HAS_ERRORS) {
        return FALSE;
    }

    if(result&NEEDS_MOVE) {
        ucm_moveMappings(ext, NULL);
        ucm_moveMappings(base, moveTarget);
        ucm_sortTable(base);
        ucm_sortTable(ext);
        if(moveTarget!=NULL) {
            ucm_sortTable(moveTarget);
        }
    }

    return TRUE;
}



U_CAPI void U_EXPORT2
ucm_mergeTables(UCMTable *fromUTable, UCMTable *toUTable,
                const uint8_t *subchar, int32_t subcharLength,
                uint8_t subchar1) {
    UCMapping *fromUMapping, *toUMapping;
    int32_t fromUIndex, toUIndex, fromUTop, toUTop, cmp;

    ucm_sortTable(fromUTable);
    ucm_sortTable(toUTable);

    fromUMapping=fromUTable->mappings;
    toUMapping=toUTable->mappings;

    fromUTop=fromUTable->mappingsLength;
    toUTop=toUTable->mappingsLength;

    fromUIndex=toUIndex=0;

    while(fromUIndex<fromUTop && toUIndex<toUTop) {
        cmp=compareMappings(fromUTable, fromUMapping, toUTable, toUMapping, TRUE);
        if(cmp==0) {
            
            ++fromUMapping;
            ++toUMapping;

            ++fromUIndex;
            ++toUIndex;
        } else if(cmp<0) {
            



            if( (fromUMapping->bLen==subcharLength &&
                 0==uprv_memcmp(UCM_GET_BYTES(fromUTable, fromUMapping), subchar, subcharLength)) ||
                (subchar1!=0 && fromUMapping->bLen==1 && fromUMapping->b.bytes[0]==subchar1)
            ) {
                fromUMapping->f=2; 
            } else {
                fromUMapping->f=1; 
            }

            ++fromUMapping;
            ++fromUIndex;
        } else {
            




            
            if(!(toUMapping->uLen==1 && (toUMapping->u==0xfffd || toUMapping->u==0x1a))) {
                toUMapping->f=3; 
                ucm_addMapping(fromUTable, toUMapping, UCM_GET_CODE_POINTS(toUTable, toUMapping), UCM_GET_BYTES(toUTable, toUMapping));

                
                fromUMapping=fromUTable->mappings+fromUIndex;
            }

            ++toUMapping;
            ++toUIndex;
        }
    }

    
    while(fromUIndex<fromUTop) {
        
        if( (fromUMapping->bLen==subcharLength &&
             0==uprv_memcmp(UCM_GET_BYTES(fromUTable, fromUMapping), subchar, subcharLength)) ||
            (subchar1!=0 && fromUMapping->bLen==1 && fromUMapping->b.bytes[0]==subchar1)
        ) {
            fromUMapping->f=2; 
        } else {
            fromUMapping->f=1; 
        }

        ++fromUMapping;
        ++fromUIndex;
    }

    while(toUIndex<toUTop) {
        

        
        if(!(toUMapping->uLen==1 && (toUMapping->u==0xfffd || toUMapping->u==0x1a))) {
            toUMapping->f=3; 
            ucm_addMapping(fromUTable, toUMapping, UCM_GET_CODE_POINTS(toUTable, toUMapping), UCM_GET_BYTES(toUTable, toUMapping));
        }

        ++toUMapping;
        ++toUIndex;
    }

    fromUTable->isSorted=FALSE;
}



U_CAPI UBool U_EXPORT2
ucm_separateMappings(UCMFile *ucm, UBool isSISO) {
    UCMTable *table;
    UCMapping *m, *mLimit;
    int32_t type;
    UBool needsMove, isOK;

    table=ucm->base;
    m=table->mappings;
    mLimit=m+table->mappingsLength;

    needsMove=FALSE;
    isOK=TRUE;

    for(; m<mLimit; ++m) {
        if(isSISO && m->bLen==1 && (m->b.bytes[0]==0xe || m->b.bytes[0]==0xf)) {
            fprintf(stderr, "warning: removing illegal mapping from an SI/SO-stateful table\n");
            ucm_printMapping(table, m, stderr);
            m->moveFlag|=UCM_REMOVE_MAPPING;
            needsMove=TRUE;
            continue;
        }

        type=ucm_mappingType(
                &ucm->states, m,
                UCM_GET_CODE_POINTS(table, m), UCM_GET_BYTES(table, m));
        if(type<0) {
            
            printMapping(m, UCM_GET_CODE_POINTS(table, m), UCM_GET_BYTES(table, m), stderr);
            isOK=FALSE;
        } else if(type>0) {
            m->moveFlag|=UCM_MOVE_TO_EXT;
            needsMove=TRUE;
        }
    }

    if(!isOK) {
        return FALSE;
    }
    if(needsMove) {
        ucm_moveMappings(ucm->base, ucm->ext);
        return ucm_checkBaseExt(&ucm->states, ucm->base, ucm->ext, ucm->ext, FALSE);
    } else {
        ucm_sortTable(ucm->base);
        return TRUE;
    }
}



U_CAPI int8_t U_EXPORT2
ucm_parseBytes(uint8_t bytes[UCNV_EXT_MAX_BYTES], const char *line, const char **ps) {
    const char *s=*ps;
    char *end;
    uint8_t byte;
    int8_t bLen;

    bLen=0;
    for(;;) {
        
        if(bLen>0 && *s=='+') {
            ++s;
        }
        if(*s!='\\') {
            break;
        }

        if( s[1]!='x' ||
            (byte=(uint8_t)uprv_strtoul(s+2, &end, 16), end)!=s+4
        ) {
            fprintf(stderr, "ucm error: byte must be formatted as \\xXX (2 hex digits) - \"%s\"\n", line);
            return -1;
        }

        if(bLen==UCNV_EXT_MAX_BYTES) {
            fprintf(stderr, "ucm error: too many bytes on \"%s\"\n", line);
            return -1;
        }
        bytes[bLen++]=byte;
        s=end;
    }

    *ps=s;
    return bLen;
}


U_CAPI UBool U_EXPORT2
ucm_parseMappingLine(UCMapping *m,
                     UChar32 codePoints[UCNV_EXT_MAX_UCHARS],
                     uint8_t bytes[UCNV_EXT_MAX_BYTES],
                     const char *line) {
    const char *s;
    char *end;
    UChar32 cp;
    int32_t u16Length;
    int8_t uLen, bLen, f;

    s=line;
    uLen=bLen=0;

    
    for(;;) {
        
        if(uLen>0 && *s=='+') {
            ++s;
        }
        if(*s!='<') {
            break;
        }

        if( s[1]!='U' ||
            (cp=(UChar32)uprv_strtoul(s+2, &end, 16), end)==s+2 ||
            *end!='>'
        ) {
            fprintf(stderr, "ucm error: Unicode code point must be formatted as <UXXXX> (1..6 hex digits) - \"%s\"\n", line);
            return FALSE;
        }
        if((uint32_t)cp>0x10ffff || U_IS_SURROGATE(cp)) {
            fprintf(stderr, "ucm error: Unicode code point must be 0..d7ff or e000..10ffff - \"%s\"\n", line);
            return FALSE;
        }

        if(uLen==UCNV_EXT_MAX_UCHARS) {
            fprintf(stderr, "ucm error: too many code points on \"%s\"\n", line);
            return FALSE;
        }
        codePoints[uLen++]=cp;
        s=end+1;
    }

    if(uLen==0) {
        fprintf(stderr, "ucm error: no Unicode code points on \"%s\"\n", line);
        return FALSE;
    } else if(uLen==1) {
        m->u=codePoints[0];
    } else {
        UErrorCode errorCode=U_ZERO_ERROR;
        u_strFromUTF32(NULL, 0, &u16Length, codePoints, uLen, &errorCode);
        if( (U_FAILURE(errorCode) && errorCode!=U_BUFFER_OVERFLOW_ERROR) ||
            u16Length>UCNV_EXT_MAX_UCHARS
        ) {
            fprintf(stderr, "ucm error: too many UChars on \"%s\"\n", line);
            return FALSE;
        }
    }

    s=u_skipWhitespace(s);

    
    bLen=ucm_parseBytes(bytes, line, &s);

    if(bLen<0) {
        return FALSE;
    } else if(bLen==0) {
        fprintf(stderr, "ucm error: no bytes on \"%s\"\n", line);
        return FALSE;
    } else if(bLen<=4) {
        uprv_memcpy(m->b.bytes, bytes, bLen);
    }

    
    for(;;) {
        if(*s==0) {
            f=-1; 
            break;
        } else if(*s=='|') {
            f=(int8_t)(s[1]-'0');
            if((uint8_t)f>3) {
                fprintf(stderr, "ucm error: fallback indicator must be |0..|3 - \"%s\"\n", line);
                return FALSE;
            }
            break;
        }
        ++s;
    }

    m->uLen=uLen;
    m->bLen=bLen;
    m->f=f;
    return TRUE;
}



U_CAPI UCMTable * U_EXPORT2
ucm_openTable() {
    UCMTable *table=(UCMTable *)uprv_malloc(sizeof(UCMTable));
    if(table==NULL) {
        fprintf(stderr, "ucm error: unable to allocate a UCMTable\n");
        exit(U_MEMORY_ALLOCATION_ERROR);
    }

    memset(table, 0, sizeof(UCMTable));
    return table;
}

U_CAPI void U_EXPORT2
ucm_closeTable(UCMTable *table) {
    if(table!=NULL) {
        uprv_free(table->mappings);
        uprv_free(table->codePoints);
        uprv_free(table->bytes);
        uprv_free(table->reverseMap);
        uprv_free(table);
    }
}

U_CAPI void U_EXPORT2
ucm_resetTable(UCMTable *table) {
    if(table!=NULL) {
        table->mappingsLength=0;
        table->flagsType=0;
        table->unicodeMask=0;
        table->bytesLength=table->codePointsLength=0;
        table->isSorted=FALSE;
    }
}

U_CAPI void U_EXPORT2
ucm_addMapping(UCMTable *table,
               UCMapping *m,
               UChar32 codePoints[UCNV_EXT_MAX_UCHARS],
               uint8_t bytes[UCNV_EXT_MAX_BYTES]) {
    UCMapping *tm;
    UChar32 c;
    int32_t idx;

    if(table->mappingsLength>=table->mappingsCapacity) {
        
        if(table->mappingsCapacity==0) {
            table->mappingsCapacity=1000;
        } else {
            table->mappingsCapacity*=10;
        }
        table->mappings=(UCMapping *)uprv_realloc(table->mappings,
                                             table->mappingsCapacity*sizeof(UCMapping));
        if(table->mappings==NULL) {
            fprintf(stderr, "ucm error: unable to allocate %d UCMappings\n",
                            (int)table->mappingsCapacity);
            exit(U_MEMORY_ALLOCATION_ERROR);
        }

        if(table->reverseMap!=NULL) {
            
            uprv_free(table->reverseMap);
            table->reverseMap=NULL;
        }
    }

    if(m->uLen>1 && table->codePointsCapacity==0) {
        table->codePointsCapacity=10000;
        table->codePoints=(UChar32 *)uprv_malloc(table->codePointsCapacity*4);
        if(table->codePoints==NULL) {
            fprintf(stderr, "ucm error: unable to allocate %d UChar32s\n",
                            (int)table->codePointsCapacity);
            exit(U_MEMORY_ALLOCATION_ERROR);
        }
    }

    if(m->bLen>4 && table->bytesCapacity==0) {
        table->bytesCapacity=10000;
        table->bytes=(uint8_t *)uprv_malloc(table->bytesCapacity);
        if(table->bytes==NULL) {
            fprintf(stderr, "ucm error: unable to allocate %d bytes\n",
                            (int)table->bytesCapacity);
            exit(U_MEMORY_ALLOCATION_ERROR);
        }
    }

    if(m->uLen>1) {
        idx=table->codePointsLength;
        table->codePointsLength+=m->uLen;
        if(table->codePointsLength>table->codePointsCapacity) {
            fprintf(stderr, "ucm error: too many code points in multiple-code point mappings\n");
            exit(U_MEMORY_ALLOCATION_ERROR);
        }

        uprv_memcpy(table->codePoints+idx, codePoints, m->uLen*4);
        m->u=idx;
    }

    if(m->bLen>4) {
        idx=table->bytesLength;
        table->bytesLength+=m->bLen;
        if(table->bytesLength>table->bytesCapacity) {
            fprintf(stderr, "ucm error: too many bytes in mappings with >4 charset bytes\n");
            exit(U_MEMORY_ALLOCATION_ERROR);
        }

        uprv_memcpy(table->bytes+idx, bytes, m->bLen);
        m->b.idx=idx;
    }

    
    for(idx=0; idx<m->uLen; ++idx) {
        c=codePoints[idx];
        if(c>=0x10000) {
            table->unicodeMask|=UCNV_HAS_SUPPLEMENTARY; 
        } else if(U_IS_SURROGATE(c)) {
            table->unicodeMask|=UCNV_HAS_SURROGATES;    
        }
    }

    
    if(m->f<0) {
        table->flagsType|=UCM_FLAGS_IMPLICIT;
    } else {
        table->flagsType|=UCM_FLAGS_EXPLICIT;
    }

    tm=table->mappings+table->mappingsLength++;
    uprv_memcpy(tm, m, sizeof(UCMapping));

    table->isSorted=FALSE;
}

U_CAPI UCMFile * U_EXPORT2
ucm_open() {
    UCMFile *ucm=(UCMFile *)uprv_malloc(sizeof(UCMFile));
    if(ucm==NULL) {
        fprintf(stderr, "ucm error: unable to allocate a UCMFile\n");
        exit(U_MEMORY_ALLOCATION_ERROR);
    }

    memset(ucm, 0, sizeof(UCMFile));

    ucm->base=ucm_openTable();
    ucm->ext=ucm_openTable();

    ucm->states.stateFlags[0]=MBCS_STATE_FLAG_DIRECT;
    ucm->states.conversionType=UCNV_UNSUPPORTED_CONVERTER;
    ucm->states.outputType=-1;
    ucm->states.minCharLength=ucm->states.maxCharLength=1;

    return ucm;
}

U_CAPI void U_EXPORT2
ucm_close(UCMFile *ucm) {
    if(ucm!=NULL) {
        ucm_closeTable(ucm->base);
        ucm_closeTable(ucm->ext);
        uprv_free(ucm);
    }
}

U_CAPI int32_t U_EXPORT2
ucm_mappingType(UCMStates *baseStates,
                UCMapping *m,
                UChar32 codePoints[UCNV_EXT_MAX_UCHARS],
                uint8_t bytes[UCNV_EXT_MAX_BYTES]) {
    
    int32_t count=ucm_countChars(baseStates, bytes, m->bLen);
    if(count<1) {
        
        return -1;
    }

    





















    if( m->uLen==1 && count==1 &&
        (baseStates->maxCharLength==1 ||
            !((m->f==2 && m->bLen==1) ||
              (m->f==1 && bytes[0]==0) ||
              (m->f<=1 && m->bLen>1 && bytes[0]==0)))
    ) {
        return 0; 
    } else {
        return 1; 
    }
}

U_CAPI UBool U_EXPORT2
ucm_addMappingAuto(UCMFile *ucm, UBool forBase, UCMStates *baseStates,
                   UCMapping *m,
                   UChar32 codePoints[UCNV_EXT_MAX_UCHARS],
                   uint8_t bytes[UCNV_EXT_MAX_BYTES]) {
    int32_t type;

    if(m->f==2 && m->uLen>1) {
        fprintf(stderr, "ucm error: illegal <subchar1> |2 mapping from multiple code points\n");
        printMapping(m, codePoints, bytes, stderr);
        return FALSE;
    }

    if(baseStates!=NULL) {
        
        type=ucm_mappingType(baseStates, m, codePoints, bytes);
        if(type<0) {
            
            printMapping(m, codePoints, bytes, stderr);
            return FALSE;
        }
    } else {
        
        type=1;
    }

    



    if(forBase && type==0) {
        ucm_addMapping(ucm->base, m, codePoints, bytes);
    } else {
        ucm_addMapping(ucm->ext, m, codePoints, bytes);
    }

    return TRUE;
}

U_CAPI UBool U_EXPORT2
ucm_addMappingFromLine(UCMFile *ucm, const char *line, UBool forBase, UCMStates *baseStates) {
    UCMapping m={ 0 };
    UChar32 codePoints[UCNV_EXT_MAX_UCHARS];
    uint8_t bytes[UCNV_EXT_MAX_BYTES];

    const char *s;

    
    if(line[0]=='#' || *(s=u_skipWhitespace(line))==0 || *s=='\n' || *s=='\r') {
        return TRUE;
    }

    return
        ucm_parseMappingLine(&m, codePoints, bytes, line) &&
        ucm_addMappingAuto(ucm, forBase, baseStates, &m, codePoints, bytes);
}

U_CAPI void U_EXPORT2
ucm_readTable(UCMFile *ucm, FileStream* convFile,
              UBool forBase, UCMStates *baseStates,
              UErrorCode *pErrorCode) {
    char line[500];
    char *end;
    UBool isOK;
    
    if(U_FAILURE(*pErrorCode)) {
        return;
    }

    isOK=TRUE;

    for(;;) {
        
        if(!T_FileStream_readLine(convFile, line, sizeof(line))) {
            fprintf(stderr, "incomplete charmap section\n");
            isOK=FALSE;
            break;
        }

        
        end=uprv_strchr(line, 0);
        while(line<end && (*(end-1)=='\r' || *(end-1)=='\n')) {
            --end;
        }
        *end=0;

        
        if(line[0]==0 || line[0]=='#') {
            continue;
        }

        
        if(0==uprv_strcmp(line, "END CHARMAP")) {
            break;
        }

        isOK&=ucm_addMappingFromLine(ucm, line, forBase, baseStates);
    }

    if(!isOK) {
        *pErrorCode=U_INVALID_TABLE_FORMAT;
    }
}
#endif
