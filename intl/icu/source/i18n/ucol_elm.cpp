
























#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

#include "unicode/uchar.h"
#include "unicode/unistr.h"
#include "unicode/ucoleitr.h"
#include "unicode/normlzr.h"
#include "unicode/utf16.h"
#include "normalizer2impl.h"
#include "ucol_elm.h"
#include "ucol_tok.h"
#include "ucol_cnt.h"
#include "unicode/caniter.h"
#include "cmemory.h"
#include "uassert.h"

U_NAMESPACE_USE

static uint32_t uprv_uca_processContraction(CntTable *contractions, UCAElements *element, uint32_t existingCE, UErrorCode *status);

U_CDECL_BEGIN
static int32_t U_CALLCONV
prefixLookupHash(const UHashTok e) {
    UCAElements *element = (UCAElements *)e.pointer;
    UChar buf[256];
    UHashTok key;
    key.pointer = buf;
    uprv_memcpy(buf, element->cPoints, element->cSize*sizeof(UChar));
    buf[element->cSize] = 0;
    
    
    return uhash_hashUChars(key);
}

static int8_t U_CALLCONV
prefixLookupComp(const UHashTok e1, const UHashTok e2) {
    UCAElements *element1 = (UCAElements *)e1.pointer;
    UCAElements *element2 = (UCAElements *)e2.pointer;

    UChar buf1[256];
    UHashTok key1;
    key1.pointer = buf1;
    uprv_memcpy(buf1, element1->cPoints, element1->cSize*sizeof(UChar));
    buf1[element1->cSize] = 0;

    UChar buf2[256];
    UHashTok key2;
    key2.pointer = buf2;
    uprv_memcpy(buf2, element2->cPoints, element2->cSize*sizeof(UChar));
    buf2[element2->cSize] = 0;

    return uhash_compareUChars(key1, key2);
}
U_CDECL_END

static int32_t uprv_uca_addExpansion(ExpansionTable *expansions, uint32_t value, UErrorCode *status) {
    if(U_FAILURE(*status)) {
        return 0;
    }
    if(expansions->CEs == NULL) {
        expansions->CEs = (uint32_t *)uprv_malloc(INIT_EXP_TABLE_SIZE*sizeof(uint32_t));
        
        if (expansions->CEs == NULL) {
            *status = U_MEMORY_ALLOCATION_ERROR;
            return 0;
        }
        expansions->size = INIT_EXP_TABLE_SIZE;
        expansions->position = 0;
    }

    if(expansions->position == expansions->size) {
        uint32_t *newData = (uint32_t *)uprv_realloc(expansions->CEs, 2*expansions->size*sizeof(uint32_t));
        if(newData == NULL) {
#ifdef UCOL_DEBUG
            fprintf(stderr, "out of memory for expansions\n");
#endif
            *status = U_MEMORY_ALLOCATION_ERROR;
            return -1;
        }
        expansions->CEs = newData;
        expansions->size *= 2;
    }

    expansions->CEs[expansions->position] = value;
    return(expansions->position++);
}

U_CAPI tempUCATable*  U_EXPORT2
uprv_uca_initTempTable(UCATableHeader *image, UColOptionSet *opts, const UCollator *UCA, UColCETags initTag, UColCETags supplementaryInitTag, UErrorCode *status) {
    MaxJamoExpansionTable *maxjet;
    MaxExpansionTable *maxet;
    tempUCATable *t = (tempUCATable *)uprv_malloc(sizeof(tempUCATable));
    
    if (t == NULL) {
        *status = U_MEMORY_ALLOCATION_ERROR;
        return NULL;
    }
    uprv_memset(t, 0, sizeof(tempUCATable));

    maxet  = (MaxExpansionTable *)uprv_malloc(sizeof(MaxExpansionTable));
    if (maxet == NULL) {
        goto allocation_failure;
    }
    uprv_memset(maxet, 0, sizeof(MaxExpansionTable));
    t->maxExpansions       = maxet;

    maxjet = (MaxJamoExpansionTable *)uprv_malloc(sizeof(MaxJamoExpansionTable));
    if (maxjet == NULL) {
        goto allocation_failure;
    }
    uprv_memset(maxjet, 0, sizeof(MaxJamoExpansionTable));
    t->maxJamoExpansions = maxjet;

    t->image = image;
    t->options = opts;

    t->UCA = UCA;
    t->expansions = (ExpansionTable *)uprv_malloc(sizeof(ExpansionTable));
    
    if (t->expansions == NULL) {
        goto allocation_failure;
    }
    uprv_memset(t->expansions, 0, sizeof(ExpansionTable));

    t->mapping = utrie_open(NULL, NULL, UCOL_ELM_TRIE_CAPACITY,
        UCOL_SPECIAL_FLAG | (initTag<<24),
        UCOL_SPECIAL_FLAG | (supplementaryInitTag << 24),
        TRUE); 
    if (U_FAILURE(*status)) {
        goto allocation_failure;
    }
    t->prefixLookup = uhash_open(prefixLookupHash, prefixLookupComp, NULL, status);
    if (U_FAILURE(*status)) {
        goto allocation_failure;
    }
    uhash_setValueDeleter(t->prefixLookup, uprv_free);

    t->contractions = uprv_cnttab_open(t->mapping, status);
    if (U_FAILURE(*status)) {
        goto cleanup;
    }

    
    if (UCA != NULL) {
        
        maxet->size            = (int32_t)(UCA->lastEndExpansionCE - UCA->endExpansionCE) + 2;
        maxet->position        = maxet->size - 1;
        maxet->endExpansionCE  = 
            (uint32_t *)uprv_malloc(sizeof(uint32_t) * maxet->size);
        
        if (maxet->endExpansionCE == NULL) {
            goto allocation_failure;
        }
        maxet->expansionCESize =
            (uint8_t *)uprv_malloc(sizeof(uint8_t) * maxet->size);
        
        if (maxet->expansionCESize == NULL) {
            goto allocation_failure;
        }
        
        *(maxet->endExpansionCE)  = 0;
        *(maxet->expansionCESize) = 0;
        uprv_memcpy(maxet->endExpansionCE + 1, UCA->endExpansionCE, 
            sizeof(uint32_t) * (maxet->size - 1));
        uprv_memcpy(maxet->expansionCESize + 1, UCA->expansionCESize, 
            sizeof(uint8_t) * (maxet->size - 1));
    }
    else {
        maxet->size     = 0;
    }
    maxjet->endExpansionCE = NULL;
    maxjet->isV = NULL;
    maxjet->size = 0;
    maxjet->position = 0;
    maxjet->maxLSize = 1;
    maxjet->maxVSize = 1;
    maxjet->maxTSize = 1;

    t->unsafeCP = (uint8_t *)uprv_malloc(UCOL_UNSAFECP_TABLE_SIZE);
    
    if (t->unsafeCP == NULL) {
        goto allocation_failure;
    }
    t->contrEndCP = (uint8_t *)uprv_malloc(UCOL_UNSAFECP_TABLE_SIZE);
    
    if (t->contrEndCP == NULL) {
        goto allocation_failure;
    }
    uprv_memset(t->unsafeCP, 0, UCOL_UNSAFECP_TABLE_SIZE);
    uprv_memset(t->contrEndCP, 0, UCOL_UNSAFECP_TABLE_SIZE);
    t->cmLookup = NULL;
    return t;

allocation_failure:
    *status = U_MEMORY_ALLOCATION_ERROR;
cleanup:
    uprv_uca_closeTempTable(t);
    return NULL;
}

static tempUCATable* U_EXPORT2
uprv_uca_cloneTempTable(tempUCATable *t, UErrorCode *status) {
    if(U_FAILURE(*status)) {
        return NULL;
    }

    tempUCATable *r = (tempUCATable *)uprv_malloc(sizeof(tempUCATable));
    
    if (r == NULL) {
        *status = U_MEMORY_ALLOCATION_ERROR;
        return NULL;
    }
    uprv_memset(r, 0, sizeof(tempUCATable));

    
    if(t->mapping != NULL) {
        
        r->mapping = utrie_clone(NULL, t->mapping, NULL, 0);
    }

    
    
    r->prefixLookup = NULL; 

    
    if(t->expansions != NULL) {
        r->expansions = (ExpansionTable *)uprv_malloc(sizeof(ExpansionTable));
        
        if (r->expansions == NULL) {
            *status = U_MEMORY_ALLOCATION_ERROR;
            goto cleanup;
        }
        r->expansions->position = t->expansions->position;
        r->expansions->size = t->expansions->size;
        if(t->expansions->CEs != NULL) {
            r->expansions->CEs = (uint32_t *)uprv_malloc(sizeof(uint32_t)*t->expansions->size);
            
            if (r->expansions->CEs == NULL) {
                *status = U_MEMORY_ALLOCATION_ERROR;
                goto cleanup;
            }
            uprv_memcpy(r->expansions->CEs, t->expansions->CEs, sizeof(uint32_t)*t->expansions->position);
        } else {
            r->expansions->CEs = NULL;
        }
    }

    if(t->contractions != NULL) {
        r->contractions = uprv_cnttab_clone(t->contractions, status);
        
        if (r->contractions == NULL) {
            *status = U_MEMORY_ALLOCATION_ERROR;
            goto cleanup;
        }
        r->contractions->mapping = r->mapping;
    }

    if(t->maxExpansions != NULL) {
        r->maxExpansions = (MaxExpansionTable *)uprv_malloc(sizeof(MaxExpansionTable));
        
        if (r->maxExpansions == NULL) {
            *status = U_MEMORY_ALLOCATION_ERROR;
            goto cleanup;
        }
        r->maxExpansions->size = t->maxExpansions->size;
        r->maxExpansions->position = t->maxExpansions->position;
        if(t->maxExpansions->endExpansionCE != NULL) {
            r->maxExpansions->endExpansionCE = (uint32_t *)uprv_malloc(sizeof(uint32_t)*t->maxExpansions->size);
            
            if (r->maxExpansions->endExpansionCE == NULL) {
                *status = U_MEMORY_ALLOCATION_ERROR;
                goto cleanup;
            }
            uprv_memset(r->maxExpansions->endExpansionCE, 0xDB, sizeof(uint32_t)*t->maxExpansions->size);
            uprv_memcpy(r->maxExpansions->endExpansionCE, t->maxExpansions->endExpansionCE, t->maxExpansions->position*sizeof(uint32_t));
        } else {
            r->maxExpansions->endExpansionCE = NULL;
        }
        if(t->maxExpansions->expansionCESize != NULL) {
            r->maxExpansions->expansionCESize = (uint8_t *)uprv_malloc(sizeof(uint8_t)*t->maxExpansions->size);
            
            if (r->maxExpansions->expansionCESize == NULL) {
                *status = U_MEMORY_ALLOCATION_ERROR;
                goto cleanup;
            }
            uprv_memset(r->maxExpansions->expansionCESize, 0xDB, sizeof(uint8_t)*t->maxExpansions->size);
            uprv_memcpy(r->maxExpansions->expansionCESize, t->maxExpansions->expansionCESize, t->maxExpansions->position*sizeof(uint8_t));
        } else {
            r->maxExpansions->expansionCESize = NULL;
        }
    }

    if(t->maxJamoExpansions != NULL) {
        r->maxJamoExpansions = (MaxJamoExpansionTable *)uprv_malloc(sizeof(MaxJamoExpansionTable));
        
        if (r->maxJamoExpansions == NULL) {
            *status = U_MEMORY_ALLOCATION_ERROR;
            goto cleanup;
        }
        r->maxJamoExpansions->size = t->maxJamoExpansions->size;
        r->maxJamoExpansions->position = t->maxJamoExpansions->position;
        r->maxJamoExpansions->maxLSize = t->maxJamoExpansions->maxLSize;
        r->maxJamoExpansions->maxVSize = t->maxJamoExpansions->maxVSize;
        r->maxJamoExpansions->maxTSize = t->maxJamoExpansions->maxTSize;
        if(t->maxJamoExpansions->size != 0) {
            r->maxJamoExpansions->endExpansionCE = (uint32_t *)uprv_malloc(sizeof(uint32_t)*t->maxJamoExpansions->size);
            
            if (r->maxJamoExpansions->endExpansionCE == NULL) {
                *status = U_MEMORY_ALLOCATION_ERROR;
                goto cleanup;
            }
            uprv_memcpy(r->maxJamoExpansions->endExpansionCE, t->maxJamoExpansions->endExpansionCE, t->maxJamoExpansions->position*sizeof(uint32_t));
            r->maxJamoExpansions->isV = (UBool *)uprv_malloc(sizeof(UBool)*t->maxJamoExpansions->size);
            
            if (r->maxJamoExpansions->isV == NULL) {
                *status = U_MEMORY_ALLOCATION_ERROR;
                goto cleanup;
            }
            uprv_memcpy(r->maxJamoExpansions->isV, t->maxJamoExpansions->isV, t->maxJamoExpansions->position*sizeof(UBool));
        } else {
            r->maxJamoExpansions->endExpansionCE = NULL;
            r->maxJamoExpansions->isV = NULL;
        }
    }

    if(t->unsafeCP != NULL) {
        r->unsafeCP = (uint8_t *)uprv_malloc(UCOL_UNSAFECP_TABLE_SIZE);
        
        if (r->unsafeCP == NULL) {
            *status = U_MEMORY_ALLOCATION_ERROR;
            goto cleanup;
        }
        uprv_memcpy(r->unsafeCP, t->unsafeCP, UCOL_UNSAFECP_TABLE_SIZE);
    }

    if(t->contrEndCP != NULL) {
        r->contrEndCP = (uint8_t *)uprv_malloc(UCOL_UNSAFECP_TABLE_SIZE);
        
        if (r->contrEndCP == NULL) {
            *status = U_MEMORY_ALLOCATION_ERROR;
            goto cleanup;
        }
        uprv_memcpy(r->contrEndCP, t->contrEndCP, UCOL_UNSAFECP_TABLE_SIZE);
    }

    r->UCA = t->UCA;
    r->image = t->image;
    r->options = t->options;

    return r;
cleanup:
    uprv_uca_closeTempTable(t);
    return NULL;
}


U_CAPI void  U_EXPORT2
uprv_uca_closeTempTable(tempUCATable *t) {
    if(t != NULL) {
        if (t->expansions != NULL) {
            uprv_free(t->expansions->CEs);
            uprv_free(t->expansions);
        }
        if(t->contractions != NULL) {
            uprv_cnttab_close(t->contractions);
        }
        if (t->mapping != NULL) {
            utrie_close(t->mapping);
        }

        if(t->prefixLookup != NULL) {
            uhash_close(t->prefixLookup);
        }

        if (t->maxExpansions != NULL) {
            uprv_free(t->maxExpansions->endExpansionCE);
            uprv_free(t->maxExpansions->expansionCESize);
            uprv_free(t->maxExpansions);
        }

        if (t->maxJamoExpansions->size > 0) {
            uprv_free(t->maxJamoExpansions->endExpansionCE);
            uprv_free(t->maxJamoExpansions->isV);
        }
        uprv_free(t->maxJamoExpansions);

        uprv_free(t->unsafeCP);
        uprv_free(t->contrEndCP);
        
        if (t->cmLookup != NULL) {
            uprv_free(t->cmLookup->cPoints);
            uprv_free(t->cmLookup);
        }

        uprv_free(t);
    }
}











static int uprv_uca_setMaxExpansion(uint32_t           endexpansion,
                                    uint8_t            expansionsize,
                                    MaxExpansionTable *maxexpansion,
                                    UErrorCode        *status)
{
    if (maxexpansion->size == 0) {
        
        maxexpansion->endExpansionCE = 
            (uint32_t *)uprv_malloc(INIT_EXP_TABLE_SIZE * sizeof(int32_t));
        
        if (maxexpansion->endExpansionCE == NULL) {
            *status = U_MEMORY_ALLOCATION_ERROR;
            return 0;
        }
        *(maxexpansion->endExpansionCE) = 0;
        maxexpansion->expansionCESize =
            (uint8_t *)uprv_malloc(INIT_EXP_TABLE_SIZE * sizeof(uint8_t));
        ;
        if (maxexpansion->expansionCESize == NULL) {
            *status = U_MEMORY_ALLOCATION_ERROR;
            return 0;
        }
        *(maxexpansion->expansionCESize) = 0;
        maxexpansion->size     = INIT_EXP_TABLE_SIZE;
        maxexpansion->position = 0;
    }

    if (maxexpansion->position + 1 == maxexpansion->size) {
        uint32_t *neweece = (uint32_t *)uprv_realloc(maxexpansion->endExpansionCE, 
            2 * maxexpansion->size * sizeof(uint32_t));
        if (neweece == NULL) {
            *status = U_MEMORY_ALLOCATION_ERROR;
            return 0;
        }
        maxexpansion->endExpansionCE  = neweece;

        uint8_t  *neweces = (uint8_t *)uprv_realloc(maxexpansion->expansionCESize, 
            2 * maxexpansion->size * sizeof(uint8_t));
        if (neweces == NULL) {
            *status = U_MEMORY_ALLOCATION_ERROR;
            return 0;
        }
        maxexpansion->expansionCESize = neweces;
        maxexpansion->size *= 2;
    }

    uint32_t *pendexpansionce = maxexpansion->endExpansionCE;
    uint8_t  *pexpansionsize  = maxexpansion->expansionCESize;
    int      pos              = maxexpansion->position;

    uint32_t *start = pendexpansionce;
    uint32_t *limit = pendexpansionce + pos;

    

    uint32_t *mid;
    int       result = -1;
    while (start < limit - 1) {
        mid = start + ((limit - start) >> 1);
        if (endexpansion <= *mid) {
            limit = mid;
        }
        else {
            start = mid;
        }
    }

    if (*start == endexpansion) {
        result = (int)(start - pendexpansionce);
    }
    else if (*limit == endexpansion) {
        result = (int)(limit - pendexpansionce);
    }

    if (result > -1) {
        

        uint8_t *currentsize = pexpansionsize + result;
        if (*currentsize < expansionsize) {
            *currentsize = expansionsize;
        }
    }
    else {
        

        
        int      shiftsize     = (int)((pendexpansionce + pos) - start);
        uint32_t *shiftpos     = start + 1;
        uint8_t  *sizeshiftpos = pexpansionsize + (shiftpos - pendexpansionce);

        
        if (shiftsize == 0 ) { 
            *(pendexpansionce + pos + 1) = endexpansion;
            *(pexpansionsize + pos + 1)  = expansionsize;
        }
        else {
            uprv_memmove(shiftpos + 1, shiftpos, shiftsize * sizeof(int32_t));
            uprv_memmove(sizeshiftpos + 1, sizeshiftpos, 
                shiftsize * sizeof(uint8_t));
            *shiftpos     = endexpansion;
            *sizeshiftpos = expansionsize;
        }
        maxexpansion->position ++;

#ifdef UCOL_DEBUG
        int   temp;
        UBool found = FALSE;
        for (temp = 0; temp < maxexpansion->position; temp ++) {
            if (pendexpansionce[temp] >= pendexpansionce[temp + 1]) {
                fprintf(stderr, "expansions %d\n", temp);
            }
            if (pendexpansionce[temp] == endexpansion) {
                found =TRUE;
                if (pexpansionsize[temp] < expansionsize) {
                    fprintf(stderr, "expansions size %d\n", temp);
                }
            }
        }
        if (pendexpansionce[temp] == endexpansion) {
            found =TRUE;
            if (pexpansionsize[temp] < expansionsize) {
                fprintf(stderr, "expansions size %d\n", temp);
            }
        }
        if (!found)
            fprintf(stderr, "expansion not found %d\n", temp);
#endif
    }

    return maxexpansion->position;
}












static int uprv_uca_setMaxJamoExpansion(UChar                  ch,
                                        uint32_t               endexpansion,
                                        uint8_t                expansionsize,
                                        MaxJamoExpansionTable *maxexpansion,
                                        UErrorCode            *status)
{
    UBool isV = TRUE;
    if (((uint32_t)ch - 0x1100) <= (0x1112 - 0x1100)) {
        

        if (maxexpansion->maxLSize < expansionsize) {
            maxexpansion->maxLSize = expansionsize;
        }
        return maxexpansion->position;
    }

    if (((uint32_t)ch - 0x1161) <= (0x1175 - 0x1161)) {
        
        if (maxexpansion->maxVSize < expansionsize) {
            maxexpansion->maxVSize = expansionsize;
        }
    }

    if (((uint32_t)ch - 0x11A8) <= (0x11C2 - 0x11A8)) {
        isV = FALSE;
        
        if (maxexpansion->maxTSize < expansionsize) {
            maxexpansion->maxTSize = expansionsize;
        }
    }

    if (maxexpansion->size == 0) {
        
        maxexpansion->endExpansionCE = 
            (uint32_t *)uprv_malloc(INIT_EXP_TABLE_SIZE * sizeof(uint32_t));
        ;
        if (maxexpansion->endExpansionCE == NULL) {
            *status = U_MEMORY_ALLOCATION_ERROR;
            return 0;
        }
        *(maxexpansion->endExpansionCE) = 0;
        maxexpansion->isV = 
            (UBool *)uprv_malloc(INIT_EXP_TABLE_SIZE * sizeof(UBool));
        ;
        if (maxexpansion->isV == NULL) {
            *status = U_MEMORY_ALLOCATION_ERROR;
            uprv_free(maxexpansion->endExpansionCE);
            maxexpansion->endExpansionCE = NULL;
            return 0;
        }
        *(maxexpansion->isV) = 0;
        maxexpansion->size     = INIT_EXP_TABLE_SIZE;
        maxexpansion->position = 0;
    }

    if (maxexpansion->position + 1 == maxexpansion->size) {
        maxexpansion->size *= 2;
        maxexpansion->endExpansionCE = (uint32_t *)uprv_realloc(maxexpansion->endExpansionCE, 
            maxexpansion->size * sizeof(uint32_t));
        if (maxexpansion->endExpansionCE == NULL) {
#ifdef UCOL_DEBUG
            fprintf(stderr, "out of memory for maxExpansions\n");
#endif
            *status = U_MEMORY_ALLOCATION_ERROR;
            return 0;
        }
        maxexpansion->isV  = (UBool *)uprv_realloc(maxexpansion->isV, 
            maxexpansion->size * sizeof(UBool));
        if (maxexpansion->isV == NULL) {
#ifdef UCOL_DEBUG
            fprintf(stderr, "out of memory for maxExpansions\n");
#endif
            *status = U_MEMORY_ALLOCATION_ERROR;
            uprv_free(maxexpansion->endExpansionCE);
            maxexpansion->endExpansionCE = NULL;
            return 0;
        }
    }

    uint32_t *pendexpansionce = maxexpansion->endExpansionCE;
    int       pos             = maxexpansion->position;

    while (pos > 0) {
        pos --;
        if (*(pendexpansionce + pos) == endexpansion) {
            return maxexpansion->position;
        }
    }

    *(pendexpansionce + maxexpansion->position) = endexpansion;
    *(maxexpansion->isV + maxexpansion->position) = isV;
    maxexpansion->position ++;

    return maxexpansion->position;
}


static void ContrEndCPSet(uint8_t *table, UChar c) {
    uint32_t    hash;
    uint8_t     *htByte;

    hash = c;
    if (hash >= UCOL_UNSAFECP_TABLE_SIZE*8) {
        hash = (hash & UCOL_UNSAFECP_TABLE_MASK) + 256;
    }
    htByte = &table[hash>>3];
    *htByte |= (1 << (hash & 7));
}


static void unsafeCPSet(uint8_t *table, UChar c) {
    uint32_t    hash;
    uint8_t     *htByte;

    hash = c;
    if (hash >= UCOL_UNSAFECP_TABLE_SIZE*8) {
        if (hash >= 0xd800 && hash <= 0xf8ff) {
            
            
            return;
        }
        hash = (hash & UCOL_UNSAFECP_TABLE_MASK) + 256;
    }
    htByte = &table[hash>>3];
    *htByte |= (1 << (hash & 7));
}

static void
uprv_uca_createCMTable(tempUCATable *t, int32_t noOfCM, UErrorCode *status) {
    t->cmLookup = (CombinClassTable *)uprv_malloc(sizeof(CombinClassTable));
    if (t->cmLookup==NULL) {
        *status = U_MEMORY_ALLOCATION_ERROR;
        return;
    }
    t->cmLookup->cPoints=(UChar *)uprv_malloc(noOfCM*sizeof(UChar));
    if (t->cmLookup->cPoints ==NULL) {
        uprv_free(t->cmLookup);
        t->cmLookup = NULL;
        *status = U_MEMORY_ALLOCATION_ERROR;
        return;
    }

    t->cmLookup->size=noOfCM;
    uprv_memset(t->cmLookup->index, 0, sizeof(t->cmLookup->index));

    return;
}

static void
uprv_uca_copyCMTable(tempUCATable *t, UChar *cm, uint16_t *index) {
    int32_t count=0;

    for (int32_t i=0; i<256; ++i) {
        if (index[i]>0) {
            
            uprv_memcpy(t->cmLookup->cPoints+count, cm+(i<<8), index[i]*sizeof(UChar));
            count += index[i];
        }
        t->cmLookup->index[i]=count;
    }
    return;
}



static void uprv_uca_unsafeCPAddCCNZ(tempUCATable *t, UErrorCode *status) {

    UChar              c;
    uint16_t           fcd;     
    UBool buildCMTable = (t->cmLookup==NULL); 
    UChar *cm=NULL;
    uint16_t index[256];
    int32_t count=0;
    const Normalizer2Impl *nfcImpl = Normalizer2Factory::getNFCImpl(*status);
    if (U_FAILURE(*status)) {
        return;
    }

    if (buildCMTable) {
        if (cm==NULL) {
            cm = (UChar *)uprv_malloc(sizeof(UChar)*UCOL_MAX_CM_TAB);
            if (cm==NULL) {
                *status = U_MEMORY_ALLOCATION_ERROR;
                return;
            }
        }
        uprv_memset(index, 0, sizeof(index));
    }
    for (c=0; c<0xffff; c++) {
        if (U16_IS_LEAD(c)) {
            fcd = 0;
            if (nfcImpl->singleLeadMightHaveNonZeroFCD16(c)) {
                UChar32 supp = U16_GET_SUPPLEMENTARY(c, 0xdc00);
                UChar32 suppLimit = supp + 0x400;
                while (supp < suppLimit) {
                    fcd |= nfcImpl->getFCD16FromNormData(supp++);
                }
            }
        } else {
            fcd = nfcImpl->getFCD16(c);
        }
        if (fcd >= 0x100 ||               
            (U16_IS_LEAD(c) && fcd != 0)) {
            if (buildCMTable) {
                uint32_t cClass = fcd & 0xff;
                
                cm[(cClass<<8)+index[cClass]] = c; 
                index[cClass]++;
                count++;
            }
            unsafeCPSet(t->unsafeCP, c);
        }
    }

    
    if (buildCMTable) {
        uprv_uca_createCMTable(t, count, status);
        if(U_FAILURE(*status)) {
            if (cm!=NULL) {
                uprv_free(cm);
            }
            return;
        }
        uprv_uca_copyCMTable(t, cm, index);
    }

    if(t->prefixLookup != NULL) {
        int32_t i = -1;
        const UHashElement *e = NULL;
        UCAElements *element = NULL;
        UChar NFCbuf[256];
        while((e = uhash_nextElement(t->prefixLookup, &i)) != NULL) {
            element = (UCAElements *)e->value.pointer;
            
            
            
            unorm_normalize(element->cPoints, element->cSize, UNORM_NFC, 0,
                NFCbuf, 256, status);
            unsafeCPSet(t->unsafeCP, NFCbuf[0]);
        }
    }

    if (cm!=NULL) {
        uprv_free(cm);
    }
}

static uint32_t uprv_uca_addPrefix(tempUCATable *t, uint32_t CE,
                                   UCAElements *element, UErrorCode *status)
{
    
    
    
    
    CntTable *contractions = t->contractions;
    UChar32 cp;
    uint32_t cpsize = 0;
    UChar *oldCP = element->cPoints;
    uint32_t oldCPSize = element->cSize;


    contractions->currentTag = SPEC_PROC_TAG;

    
    uint32_t j = 0;
#ifdef UCOL_DEBUG
    for(j=0; j<element->cSize; j++) {
        fprintf(stdout, "CP: %04X ", element->cPoints[j]);
    }
    fprintf(stdout, "El: %08X Pref: ", CE);
    for(j=0; j<element->prefixSize; j++) {
        fprintf(stdout, "%04X ", element->prefix[j]);
    }
    fprintf(stdout, "%08X ", element->mapCE);
#endif

    for (j = 1; j<element->prefixSize; j++) {   
        
        
        if(!(U16_IS_TRAIL(element->prefix[j]))) {
            unsafeCPSet(t->unsafeCP, element->prefix[j]);
        }
    }

    UChar tempPrefix = 0;

    for(j = 0; j < element->prefixSize/2; j++) { 
        
        tempPrefix = *(element->prefix+element->prefixSize-j-1);
        *(element->prefix+element->prefixSize-j-1) = element->prefix[j];
        element->prefix[j] = tempPrefix;
    }

#ifdef UCOL_DEBUG
    fprintf(stdout, "Reversed: ");
    for(j=0; j<element->prefixSize; j++) {
        fprintf(stdout, "%04X ", element->prefix[j]);
    }
    fprintf(stdout, "%08X\n", element->mapCE);
#endif

    
    if(!(U16_IS_TRAIL(element->cPoints[0]))) {
        unsafeCPSet(t->unsafeCP, element->cPoints[0]);
    }

    
    
    
    
    
    

    element->cPoints = element->prefix;
    element->cSize = element->prefixSize;

    
    
    
    if(!(U16_IS_TRAIL(element->cPoints[element->cSize -1]))) {
        ContrEndCPSet(t->contrEndCP, element->cPoints[element->cSize -1]);
    }

    
    U16_NEXT(element->cPoints, cpsize, element->cSize, cp);

    
    
    if(UCOL_ISJAMO(element->prefix[0])) {
        t->image->jamoSpecial = TRUE;
    }
    
    

    if(!isPrefix(CE)) {
        
        int32_t firstContractionOffset = 0;
        firstContractionOffset = uprv_cnttab_addContraction(contractions, UPRV_CNTTAB_NEWELEMENT, 0, CE, status);
        uint32_t newCE = uprv_uca_processContraction(contractions, element, UCOL_NOT_FOUND, status);
        uprv_cnttab_addContraction(contractions, firstContractionOffset, *element->prefix, newCE, status);
        uprv_cnttab_addContraction(contractions, firstContractionOffset, 0xFFFF, CE, status);
        CE =  constructContractCE(SPEC_PROC_TAG, firstContractionOffset);
    } else { 
        
        
        int32_t position = uprv_cnttab_findCP(contractions, CE, *element->prefix, status);
        if(position > 0) {       
            uint32_t eCE = uprv_cnttab_getCE(contractions, CE, position, status);
            uint32_t newCE = uprv_uca_processContraction(contractions, element, eCE, status);
            uprv_cnttab_setContraction(contractions, CE, position, *(element->prefix), newCE, status);
        } else {                  
            uprv_uca_processContraction(contractions, element, UCOL_NOT_FOUND, status);
            uprv_cnttab_insertContraction(contractions, CE, *(element->prefix), element->mapCE, status);
        }
    }

    element->cPoints = oldCP;
    element->cSize = oldCPSize;

    return CE;
}






static uint32_t uprv_uca_addContraction(tempUCATable *t, uint32_t CE, 
                                        UCAElements *element, UErrorCode *status)
{
    CntTable *contractions = t->contractions;
    UChar32 cp;
    uint32_t cpsize = 0;

    contractions->currentTag = CONTRACTION_TAG;

    
    U16_NEXT(element->cPoints, cpsize, element->cSize, cp);

    if(cpsize<element->cSize) { 
        uint32_t j = 0;
        for (j=1; j<element->cSize; j++) {   
            
            
            if(!(U16_IS_TRAIL(element->cPoints[j]))) {
                unsafeCPSet(t->unsafeCP, element->cPoints[j]);
            }
        }
        
        
        
        if(!(U16_IS_TRAIL(element->cPoints[element->cSize -1]))) {
            ContrEndCPSet(t->contrEndCP, element->cPoints[element->cSize -1]);
        }

        
        
        if(UCOL_ISJAMO(element->cPoints[0])) {
            t->image->jamoSpecial = TRUE;
        }
        
        
        element->cPoints+=cpsize;
        element->cSize-=cpsize;
        if(!isContraction(CE)) { 
            
            int32_t firstContractionOffset = 0;
            firstContractionOffset = uprv_cnttab_addContraction(contractions, UPRV_CNTTAB_NEWELEMENT, 0, CE, status);
            uint32_t newCE = uprv_uca_processContraction(contractions, element, UCOL_NOT_FOUND, status);
            uprv_cnttab_addContraction(contractions, firstContractionOffset, *element->cPoints, newCE, status);
            uprv_cnttab_addContraction(contractions, firstContractionOffset, 0xFFFF, CE, status);
            CE =  constructContractCE(CONTRACTION_TAG, firstContractionOffset);
        } else { 
            
            
            int32_t position = uprv_cnttab_findCP(contractions, CE, *element->cPoints, status);
            if(position > 0) {       
                uint32_t eCE = uprv_cnttab_getCE(contractions, CE, position, status);
                uint32_t newCE = uprv_uca_processContraction(contractions, element, eCE, status);
                uprv_cnttab_setContraction(contractions, CE, position, *(element->cPoints), newCE, status);
            } else {                  
                uint32_t newCE = uprv_uca_processContraction(contractions, element, UCOL_NOT_FOUND, status);
                uprv_cnttab_insertContraction(contractions, CE, *(element->cPoints), newCE, status);
            }
        }
        element->cPoints-=cpsize;
        element->cSize+=cpsize;
        
        utrie_set32(t->mapping, cp, CE);
    } else if(!isContraction(CE)) { 
        
        utrie_set32(t->mapping, cp, element->mapCE);
    } else { 
        uprv_cnttab_changeContraction(contractions, CE, 0, element->mapCE, status);
        uprv_cnttab_changeContraction(contractions, CE, 0xFFFF, element->mapCE, status);
    }
    return CE;
}


static uint32_t uprv_uca_processContraction(CntTable *contractions, UCAElements *element, uint32_t existingCE, UErrorCode *status) {
    int32_t firstContractionOffset = 0;
    

    if(U_FAILURE(*status)) {
        return UCOL_NOT_FOUND;
    }

    
    if(element->cSize == 1) {
        if(isCntTableElement(existingCE) && ((UColCETags)getCETag(existingCE) == contractions->currentTag)) {
            uprv_cnttab_changeContraction(contractions, existingCE, 0, element->mapCE, status);
            uprv_cnttab_changeContraction(contractions, existingCE, 0xFFFF, element->mapCE, status);
            return existingCE;
        } else {
            return element->mapCE; 
        }
    }

    
    

    
    
    element->cPoints++;
    element->cSize--;
    if(!isCntTableElement(existingCE)) { 
        
        firstContractionOffset = uprv_cnttab_addContraction(contractions, UPRV_CNTTAB_NEWELEMENT, 0, existingCE, status);
        uint32_t newCE = uprv_uca_processContraction(contractions, element, UCOL_NOT_FOUND, status);
        uprv_cnttab_addContraction(contractions, firstContractionOffset, *element->cPoints, newCE, status);
        uprv_cnttab_addContraction(contractions, firstContractionOffset, 0xFFFF, existingCE, status);
        existingCE =  constructContractCE(contractions->currentTag, firstContractionOffset);
    } else { 
        
        
        int32_t position = uprv_cnttab_findCP(contractions, existingCE, *element->cPoints, status);
        if(position > 0) {       
            uint32_t eCE = uprv_cnttab_getCE(contractions, existingCE, position, status);
            uint32_t newCE = uprv_uca_processContraction(contractions, element, eCE, status);
            uprv_cnttab_setContraction(contractions, existingCE, position, *(element->cPoints), newCE, status);
        } else {                  
            uint32_t newCE = uprv_uca_processContraction(contractions, element, UCOL_NOT_FOUND, status);
            uprv_cnttab_insertContraction(contractions, existingCE, *(element->cPoints), newCE, status);
        }
    }
    element->cPoints--;
    element->cSize++;
    return existingCE;
}

static uint32_t uprv_uca_finalizeAddition(tempUCATable *t, UCAElements *element, UErrorCode *status) {
    uint32_t CE = UCOL_NOT_FOUND;
    
    
    
    uint32_t i = 0;
    if(element->mapCE == 0) {
        for(i = 0; i < element->cSize; i++) {
            if(!U16_IS_TRAIL(element->cPoints[i])) {
                unsafeCPSet(t->unsafeCP, element->cPoints[i]);
            }
        }
    }
    if(element->cSize > 1) { 
        uint32_t i = 0;
        UChar32 cp;

        U16_NEXT(element->cPoints, i, element->cSize, cp);
        
        CE = utrie_get32(t->mapping, cp, NULL);

        CE = uprv_uca_addContraction(t, CE, element, status);
    } else { 
        
        CE = utrie_get32(t->mapping, element->cPoints[0], NULL);

        if( CE != UCOL_NOT_FOUND) {
            if(isCntTableElement(CE) ) { 
                if(!isPrefix(element->mapCE)) { 
                    
                    uprv_cnttab_setContraction(t->contractions, CE, 0, 0, element->mapCE, status);
                    
                    uprv_cnttab_changeLastCE(t->contractions, CE, element->mapCE, status);
                }
            } else {
                
                utrie_set32(t->mapping, element->cPoints[0], element->mapCE);
                if ((element->prefixSize!=0) && (!isSpecial(CE) || (getCETag(CE)!=IMPLICIT_TAG))) {
                    UCAElements *origElem = (UCAElements *)uprv_malloc(sizeof(UCAElements));
                    
                    if (origElem== NULL) {
                        *status = U_MEMORY_ALLOCATION_ERROR;
                        return 0;
                    }
                    
                    origElem->prefixSize = 0;
                    origElem->prefix = NULL;
                    origElem->cPoints = origElem->uchars;
                    origElem->cPoints[0] = element->cPoints[0];
                    origElem->cSize = 1;
                    origElem->CEs[0]=CE;
                    origElem->mapCE=CE;
                    origElem->noOfCEs=1;
                    uprv_uca_finalizeAddition(t, origElem, status);
                    uprv_free(origElem);
                }
#ifdef UCOL_DEBUG
                fprintf(stderr, "Warning - trying to overwrite existing data %08X for cp %04X with %08X\n", CE, element->cPoints[0], element->CEs[0]);
                
#endif
            }
        } else {
            
            utrie_set32(t->mapping, element->cPoints[0], element->mapCE);
        }
    }
    return CE;
}


U_CAPI uint32_t  U_EXPORT2
uprv_uca_addAnElement(tempUCATable *t, UCAElements *element, UErrorCode *status) {
    U_NAMESPACE_USE

    ExpansionTable *expansions = t->expansions;

    uint32_t i = 1;
    uint32_t expansion = 0;
    uint32_t CE;

    if(U_FAILURE(*status)) {
        return 0xFFFF;
    }

    element->mapCE = 0; 

    if(element->noOfCEs == 1) {
        element->mapCE = element->CEs[0];      
    } else {     
        
        
        
        
        
        
        
        
        
        
        
        
        
        if(element->noOfCEs == 2 
            && isContinuation(element->CEs[1]) 
            && (element->CEs[1] & (~(0xFF << 24 | UCOL_CONTINUATION_MARKER))) == 0 
            && (((element->CEs[0]>>8) & 0xFF) == UCOL_BYTE_COMMON) 
            && ((element->CEs[0] & 0xFF) == UCOL_BYTE_COMMON) 
            )
        {
#ifdef UCOL_DEBUG
            fprintf(stdout, "Long primary %04X\n", element->cPoints[0]);
#endif
            element->mapCE = UCOL_SPECIAL_FLAG | (LONG_PRIMARY_TAG<<24) 
                | ((element->CEs[0]>>8) & 0xFFFF00) 
                | ((element->CEs[1]>>24) & 0xFF);   
        }
        else {
            expansion = (uint32_t)(UCOL_SPECIAL_FLAG | (EXPANSION_TAG<<UCOL_TAG_SHIFT) 
                | (((uprv_uca_addExpansion(expansions, element->CEs[0], status)+(headersize>>2))<<4)
                   & 0xFFFFF0));

            for(i = 1; i<element->noOfCEs; i++) {
                uprv_uca_addExpansion(expansions, element->CEs[i], status);
            }
            if(element->noOfCEs <= 0xF) {
                expansion |= element->noOfCEs;
            } else {
                uprv_uca_addExpansion(expansions, 0, status);
            }
            element->mapCE = expansion;
            uprv_uca_setMaxExpansion(element->CEs[element->noOfCEs - 1],
                (uint8_t)element->noOfCEs,
                t->maxExpansions,
                status);
            if(UCOL_ISJAMO(element->cPoints[0])) {
                t->image->jamoSpecial = TRUE;
                uprv_uca_setMaxJamoExpansion(element->cPoints[0],
                    element->CEs[element->noOfCEs - 1],
                    (uint8_t)element->noOfCEs,
                    t->maxJamoExpansions,
                    status);
            }
            if (U_FAILURE(*status)) {
                return 0;
            }
        }
    }

    
    
    UChar32 uniChar = 0;
    
    if ((element->cSize == 2) && U16_IS_LEAD(element->cPoints[0])){
        uniChar = U16_GET_SUPPLEMENTARY(element->cPoints[0], element->cPoints[1]);
    } else if (element->cSize == 1){
        uniChar = element->cPoints[0];
    }

    
    
    
    
    
    
    if (uniChar != 0 && u_isdigit(uniChar)){
        expansion = (uint32_t)(UCOL_SPECIAL_FLAG | (DIGIT_TAG<<UCOL_TAG_SHIFT) | 1); 
        if(element->mapCE) { 
            expansion |= ((uprv_uca_addExpansion(expansions, element->mapCE, status)+(headersize>>2))<<4);
        } else {
            expansion |= ((uprv_uca_addExpansion(expansions, element->CEs[0], status)+(headersize>>2))<<4);
        }
        element->mapCE = expansion;

        
        if(uniChar <= 0xFFFF) { 
            unsafeCPSet(t->unsafeCP, (UChar)uniChar);
        }
    }

    
    
    

    if(element->prefixSize!=0) {
        
        
        
        

        UCAElements *composed = (UCAElements *)uprv_malloc(sizeof(UCAElements));
        
        if (composed == NULL) {
            *status = U_MEMORY_ALLOCATION_ERROR;
            return 0;
        }
        uprv_memcpy(composed, element, sizeof(UCAElements));
        composed->cPoints = composed->uchars;
        composed->prefix = composed->prefixChars;

        composed->prefixSize = unorm_normalize(element->prefix, element->prefixSize, UNORM_NFC, 0, composed->prefix, 128, status);


        if(t->prefixLookup != NULL) {
            UCAElements *uCE = (UCAElements *)uhash_get(t->prefixLookup, element);
            if(uCE != NULL) { 
                element->mapCE = uprv_uca_addPrefix(t, uCE->mapCE, element, status);
            } else { 
                element->mapCE = uprv_uca_addPrefix(t, UCOL_NOT_FOUND, element, status);
                uCE = (UCAElements *)uprv_malloc(sizeof(UCAElements));
                
                if (uCE == NULL) {
                    *status = U_MEMORY_ALLOCATION_ERROR;
                    return 0;
                }
                uprv_memcpy(uCE, element, sizeof(UCAElements));
                uCE->cPoints = uCE->uchars;
                uhash_put(t->prefixLookup, uCE, uCE, status);
            }
            if(composed->prefixSize != element->prefixSize || uprv_memcmp(composed->prefix, element->prefix, element->prefixSize)) {
                
                composed->mapCE = uprv_uca_addPrefix(t, element->mapCE, composed, status);
            }
        }
        uprv_free(composed);
    }

    
    
    
    if(element->cSize > 1 && !(element->cSize==2 && U16_IS_LEAD(element->cPoints[0]) && U16_IS_TRAIL(element->cPoints[1]))) { 
        UnicodeString source(element->cPoints, element->cSize);
        CanonicalIterator it(source, *status);
        source = it.next();
        while(!source.isBogus()) {
            if(Normalizer::quickCheck(source, UNORM_FCD, *status) != UNORM_NO) {
                element->cSize = source.extract(element->cPoints, 128, *status);
                uprv_uca_finalizeAddition(t, element, status);
            }
            source = it.next();
        }
        CE = element->mapCE;
    } else {
        CE = uprv_uca_finalizeAddition(t, element, status);  
    }

    return CE;
}



static void uprv_uca_getMaxExpansionJamo(UNewTrie       *mapping, 
                                         MaxExpansionTable     *maxexpansion,
                                         MaxJamoExpansionTable *maxjamoexpansion,
                                         UBool                  jamospecial,
                                         UErrorCode            *status)
{
    const uint32_t VBASE  = 0x1161;
    const uint32_t TBASE  = 0x11A8;
    const uint32_t VCOUNT = 21;
    const uint32_t TCOUNT = 28;

    uint32_t v = VBASE + VCOUNT - 1;
    uint32_t t = TBASE + TCOUNT - 1;
    uint32_t ce;

    while (v >= VBASE) {
        
        ce = utrie_get32(mapping, v, NULL);
        if (ce < UCOL_SPECIAL_FLAG) {
            uprv_uca_setMaxExpansion(ce, 2, maxexpansion, status);
        }
        v --;
    }

    while (t >= TBASE)
    {
        
        ce = utrie_get32(mapping, t, NULL);
        if (ce < UCOL_SPECIAL_FLAG) {
            uprv_uca_setMaxExpansion(ce, 3, maxexpansion, status);
        }
        t --;
    }
    
    if (jamospecial) {
        
        int     count    = maxjamoexpansion->position;
        uint8_t maxTSize = (uint8_t)(maxjamoexpansion->maxLSize + 
            maxjamoexpansion->maxVSize +
            maxjamoexpansion->maxTSize);
        uint8_t maxVSize = (uint8_t)(maxjamoexpansion->maxLSize + 
            maxjamoexpansion->maxVSize);

        while (count > 0) {
            count --;
            if (*(maxjamoexpansion->isV + count) == TRUE) {
                uprv_uca_setMaxExpansion(
                    *(maxjamoexpansion->endExpansionCE + count), 
                    maxVSize, maxexpansion, status);
            }
            else {
                uprv_uca_setMaxExpansion(
                    *(maxjamoexpansion->endExpansionCE + count), 
                    maxTSize, maxexpansion, status);
            }
        }
    }
}

U_CDECL_BEGIN
static inline uint32_t U_CALLCONV
getFoldedValue(UNewTrie *trie, UChar32 start, int32_t offset)
{
    uint32_t value;
    uint32_t tag;
    UChar32 limit;
    UBool inBlockZero;

    limit=start+0x400;
    while(start<limit) {
        value=utrie_get32(trie, start, &inBlockZero);
        tag = getCETag(value);
        if(inBlockZero == TRUE) {
            start+=UTRIE_DATA_BLOCK_LENGTH;
        } else if(!(isSpecial(value) && (tag == IMPLICIT_TAG || tag == NOT_FOUND_TAG))) {
            



#ifdef UCOL_DEBUG
            static int32_t count = 1;
            fprintf(stdout, "%i, Folded %08X, value %08X\n", count++, start, value);
#endif
            return (uint32_t)(UCOL_SPECIAL_FLAG | (SURROGATE_TAG<<24) | offset);
        } else {
            ++start;
        }
    }
    return 0;
}
U_CDECL_END

#ifdef UCOL_DEBUG


UBool enumRange(const void *context, UChar32 start, UChar32 limit, uint32_t value) {
    if(start<0x10000) {
        fprintf(stdout, "%08X, %08X, %08X\n", start, limit, value);
    } else {
        fprintf(stdout, "%08X=%04X %04X, %08X=%04X %04X, %08X\n", start, U16_LEAD(start), U16_TRAIL(start), limit, U16_LEAD(limit), U16_TRAIL(limit), value);
    }
    return TRUE;
}

int32_t 
myGetFoldingOffset(uint32_t data) {
    if(data > UCOL_NOT_FOUND && getCETag(data) == SURROGATE_TAG) {
        return (data&0xFFFFFF);
    } else {
        return 0;
    }
}
#endif

U_CAPI UCATableHeader* U_EXPORT2
uprv_uca_assembleTable(tempUCATable *t, UErrorCode *status) {
    
    UNewTrie *mapping = t->mapping;
    ExpansionTable *expansions = t->expansions;
    CntTable *contractions = t->contractions;
    MaxExpansionTable *maxexpansion = t->maxExpansions;

    if(U_FAILURE(*status)) {
        return NULL;
    }

    uint32_t beforeContractions = (uint32_t)((headersize+paddedsize(expansions->position*sizeof(uint32_t)))/sizeof(UChar));

    int32_t contractionsSize = 0;
    contractionsSize = uprv_cnttab_constructTable(contractions, beforeContractions, status);

    
    
    
    uprv_uca_getMaxExpansionJamo(mapping, maxexpansion, t->maxJamoExpansions,
        t->image->jamoSpecial, status);

    
    
    
    

    
    int32_t mappingSize = utrie_serialize(mapping, NULL, 0, getFoldedValue , FALSE, status);

    uint32_t tableOffset = 0;
    uint8_t *dataStart;

    

    uint32_t toAllocate =(uint32_t)(headersize+                                    
        paddedsize(expansions->position*sizeof(uint32_t))+
        paddedsize(mappingSize)+
        paddedsize(contractionsSize*(sizeof(UChar)+sizeof(uint32_t)))+
        
        
        + paddedsize(maxexpansion->position * sizeof(uint32_t)) +
        
        paddedsize(maxexpansion->position * sizeof(uint8_t)) +
        paddedsize(UCOL_UNSAFECP_TABLE_SIZE) +   
        paddedsize(UCOL_UNSAFECP_TABLE_SIZE));    


    dataStart = (uint8_t *)uprv_malloc(toAllocate);
    
    if (dataStart == NULL) {
        *status = U_MEMORY_ALLOCATION_ERROR;
        return NULL;
    }

    UCATableHeader *myData = (UCATableHeader *)dataStart;
    
    uprv_memset(dataStart, 0, toAllocate);
    
    myData->magic = UCOL_HEADER_MAGIC;
    myData->isBigEndian = U_IS_BIG_ENDIAN;
    myData->charSetFamily = U_CHARSET_FAMILY;
    myData->formatVersion[0] = UCA_FORMAT_VERSION_0;
    myData->formatVersion[1] = UCA_FORMAT_VERSION_1;
    myData->formatVersion[2] = UCA_FORMAT_VERSION_2;
    myData->formatVersion[3] = UCA_FORMAT_VERSION_3;
    myData->jamoSpecial = t->image->jamoSpecial;

    
    

    myData->contractionSize = contractionsSize;

    tableOffset += (uint32_t)(paddedsize(sizeof(UCATableHeader)));

    myData->options = tableOffset;
    uprv_memcpy(dataStart+tableOffset, t->options, sizeof(UColOptionSet));
    tableOffset += (uint32_t)(paddedsize(sizeof(UColOptionSet)));

    
    
    myData->expansion = tableOffset;
    uprv_memcpy(dataStart+tableOffset, expansions->CEs, expansions->position*sizeof(uint32_t));
    tableOffset += (uint32_t)(paddedsize(expansions->position*sizeof(uint32_t)));

    
    if(contractionsSize != 0) {
        
        
        myData->contractionIndex = tableOffset;
        uprv_memcpy(dataStart+tableOffset, contractions->codePoints, contractionsSize*sizeof(UChar));
        tableOffset += (uint32_t)(paddedsize(contractionsSize*sizeof(UChar)));

        
        
        myData->contractionCEs = tableOffset;
        uprv_memcpy(dataStart+tableOffset, contractions->CEs, contractionsSize*sizeof(uint32_t));
        tableOffset += (uint32_t)(paddedsize(contractionsSize*sizeof(uint32_t)));
    } else {
        myData->contractionIndex = 0;
        myData->contractionCEs = 0;
    }

    
    
    
    

    myData->mappingPosition = tableOffset;
    utrie_serialize(mapping, dataStart+tableOffset, toAllocate-tableOffset, getFoldedValue, FALSE, status);
#ifdef UCOL_DEBUG
    
    {
        UTrie UCAt = { 0 };
        uint32_t trieWord;
        utrie_unserialize(&UCAt, dataStart+tableOffset, 9999999, status);
        UCAt.getFoldingOffset = myGetFoldingOffset;
        if(U_SUCCESS(*status)) {
            utrie_enum(&UCAt, NULL, enumRange, NULL);
        }
        trieWord = UTRIE_GET32_FROM_LEAD(&UCAt, 0xDC01);
    }
#endif
    tableOffset += paddedsize(mappingSize);


    int32_t i = 0;

    
    myData->endExpansionCE      = tableOffset;
    myData->endExpansionCECount = maxexpansion->position - 1;
    
    uprv_memcpy(dataStart + tableOffset, maxexpansion->endExpansionCE + 1, 
        (maxexpansion->position - 1) * sizeof(uint32_t));
    tableOffset += (uint32_t)(paddedsize((maxexpansion->position)* sizeof(uint32_t)));
    myData->expansionCESize = tableOffset;
    uprv_memcpy(dataStart + tableOffset, maxexpansion->expansionCESize + 1, 
        (maxexpansion->position - 1) * sizeof(uint8_t));
    tableOffset += (uint32_t)(paddedsize((maxexpansion->position)* sizeof(uint8_t)));

    
    uprv_uca_unsafeCPAddCCNZ(t, status);
    if (t->UCA != 0) {              
        for (i=0; i<UCOL_UNSAFECP_TABLE_SIZE; i++) {    
            t->unsafeCP[i] |= t->UCA->unsafeCP[i];
        }
    }
    myData->unsafeCP = tableOffset;
    uprv_memcpy(dataStart + tableOffset, t->unsafeCP, UCOL_UNSAFECP_TABLE_SIZE);
    tableOffset += paddedsize(UCOL_UNSAFECP_TABLE_SIZE);


    
    if (t->UCA != 0) {              
        for (i=0; i<UCOL_UNSAFECP_TABLE_SIZE; i++) {    
            t->contrEndCP[i] |= t->UCA->contrEndCP[i];
        }
    }
    myData->contrEndCP = tableOffset;
    uprv_memcpy(dataStart + tableOffset, t->contrEndCP, UCOL_UNSAFECP_TABLE_SIZE);
    tableOffset += paddedsize(UCOL_UNSAFECP_TABLE_SIZE);

    if(tableOffset != toAllocate) {
#ifdef UCOL_DEBUG
        fprintf(stderr, "calculation screwup!!! Expected to write %i but wrote %i instead!!!\n", toAllocate, tableOffset);
#endif
        *status = U_INTERNAL_PROGRAM_ERROR;
        uprv_free(dataStart);
        return 0;
    }

    myData->size = tableOffset;
    
    
    
    return myData;
}


struct enumStruct {
    tempUCATable *t;
    UCollator *tempColl;
    UCollationElements* colEl;
    const Normalizer2Impl *nfcImpl;
    UnicodeSet *closed;
    int32_t noOfClosures;
    UErrorCode *status;
};
U_CDECL_BEGIN
static UBool U_CALLCONV
_enumCategoryRangeClosureCategory(const void *context, UChar32 start, UChar32 limit, UCharCategory type) {

    if (type != U_UNASSIGNED && type != U_PRIVATE_USE_CHAR) { 
        UErrorCode *status = ((enumStruct *)context)->status;
        tempUCATable *t = ((enumStruct *)context)->t;
        UCollator *tempColl = ((enumStruct *)context)->tempColl;
        UCollationElements* colEl = ((enumStruct *)context)->colEl;
        UCAElements el;
        UChar decompBuffer[4];
        const UChar *decomp;
        int32_t noOfDec = 0;

        UChar32 u32 = 0;
        UChar comp[2];
        uint32_t len = 0;

        for(u32 = start; u32 < limit; u32++) {
            decomp = ((enumStruct *)context)->nfcImpl->
                getDecomposition(u32, decompBuffer, noOfDec);
            
            
            if(decomp != NULL)
            {
                len = 0;
                U16_APPEND_UNSAFE(comp, len, u32);
                if(ucol_strcoll(tempColl, comp, len, decomp, noOfDec) != UCOL_EQUAL) {
#ifdef UCOL_DEBUG
                    fprintf(stderr, "Closure: U+%04X -> ", u32);
                    UChar32 c;
                    int32_t i = 0;
                    while(i < noOfDec) {
                        U16_NEXT(decomp, i, noOfDec, c);
                        fprintf(stderr, "%04X ", c);
                    }
                    fprintf(stderr, "\n");
                    
                    fprintf(stderr, "U+%04X CEs: ", u32);
                    UCollationElements *iter = ucol_openElements(tempColl, comp, len, status);
                    int32_t ce;
                    while((ce = ucol_next(iter, status)) != UCOL_NULLORDER) {
                        fprintf(stderr, "%08X ", ce);
                    }
                    fprintf(stderr, "\nDecomp CEs: ");
                    ucol_setText(iter, decomp, noOfDec, status);
                    while((ce = ucol_next(iter, status)) != UCOL_NULLORDER) {
                        fprintf(stderr, "%08X ", ce);
                    }
                    fprintf(stderr, "\n");
                    ucol_closeElements(iter);
#endif
                    if(((enumStruct *)context)->closed != NULL) {
                        ((enumStruct *)context)->closed->add(u32);
                    }
                    ((enumStruct *)context)->noOfClosures++;
                    el.cPoints = (UChar *)decomp;
                    el.cSize = noOfDec;
                    el.noOfCEs = 0;
                    el.prefix = el.prefixChars;
                    el.prefixSize = 0;

                    UCAElements *prefix=(UCAElements *)uhash_get(t->prefixLookup, &el);
                    el.cPoints = comp;
                    el.cSize = len;
                    el.prefix = el.prefixChars;
                    el.prefixSize = 0;
                    if(prefix == NULL) {
                        el.noOfCEs = 0;
                        ucol_setText(colEl, decomp, noOfDec, status);
                        while((el.CEs[el.noOfCEs] = ucol_next(colEl, status)) != (uint32_t)UCOL_NULLORDER) {
                            el.noOfCEs++;
                        }
                    } else {
                        el.noOfCEs = 1;
                        el.CEs[0] = prefix->mapCE;
                        
                        
                        
                        
                        
                    }
                    uprv_uca_addAnElement(t, &el, status);
                }
            }
        }
    }
    return TRUE;
}
U_CDECL_END

static void
uprv_uca_setMapCE(tempUCATable *t, UCAElements *element, UErrorCode *status) {
    uint32_t expansion = 0;
    int32_t j;

    ExpansionTable *expansions = t->expansions;
    if(element->noOfCEs == 2 
        && isContinuation(element->CEs[1]) 
        && (element->CEs[1] & (~(0xFF << 24 | UCOL_CONTINUATION_MARKER))) == 0 
        && (((element->CEs[0]>>8) & 0xFF) == UCOL_BYTE_COMMON) 
        && ((element->CEs[0] & 0xFF) == UCOL_BYTE_COMMON) 
        ) {
            element->mapCE = UCOL_SPECIAL_FLAG | (LONG_PRIMARY_TAG<<24) 
                | ((element->CEs[0]>>8) & 0xFFFF00) 
                | ((element->CEs[1]>>24) & 0xFF);   
        } else {
            expansion = (uint32_t)(UCOL_SPECIAL_FLAG | (EXPANSION_TAG<<UCOL_TAG_SHIFT)
                | (((uprv_uca_addExpansion(expansions, element->CEs[0], status)+(headersize>>2))<<4)
                   & 0xFFFFF0));

            for(j = 1; j<(int32_t)element->noOfCEs; j++) {
                uprv_uca_addExpansion(expansions, element->CEs[j], status);
            }
            if(element->noOfCEs <= 0xF) {
                expansion |= element->noOfCEs;
            } else {
                uprv_uca_addExpansion(expansions, 0, status);
            }
            element->mapCE = expansion;
            uprv_uca_setMaxExpansion(element->CEs[element->noOfCEs - 1],
                (uint8_t)element->noOfCEs,
                t->maxExpansions,
                status);
        }
}

static void
uprv_uca_addFCD4AccentedContractions(tempUCATable *t,
                                      UCollationElements* colEl,
                                      UChar *data,
                                      int32_t len,
                                      UCAElements *el,
                                      UErrorCode *status) {
    UChar decomp[256], comp[256];
    int32_t decLen, compLen;

    decLen = unorm_normalize(data, len, UNORM_NFD, 0, decomp, 256, status);
    compLen = unorm_normalize(data, len, UNORM_NFC, 0, comp, 256, status);
    decomp[decLen] = comp[compLen] = 0;

    el->cPoints = decomp;
    el->cSize = decLen;
    el->noOfCEs = 0;
    el->prefixSize = 0;
    el->prefix = el->prefixChars;

    UCAElements *prefix=(UCAElements *)uhash_get(t->prefixLookup, el);
    el->cPoints = comp;
    el->cSize = compLen;
    el->prefix = el->prefixChars;
    el->prefixSize = 0;
    if(prefix == NULL) {
        el->noOfCEs = 0;
        ucol_setText(colEl, decomp, decLen, status);
        while((el->CEs[el->noOfCEs] = ucol_next(colEl, status)) != (uint32_t)UCOL_NULLORDER) {
            el->noOfCEs++;
        }
        uprv_uca_setMapCE(t, el, status);
        uprv_uca_addAnElement(t, el, status);
    }
    el->cPoints=NULL; 
}

static void
uprv_uca_addMultiCMContractions(tempUCATable *t,
                                UCollationElements* colEl,
                                tempTailorContext *c,
                                UCAElements *el,
                                UErrorCode *status) {
    CombinClassTable *cmLookup = t->cmLookup;
    UChar  newDecomp[256];
    int32_t maxComp, newDecLen;
    const Normalizer2Impl *nfcImpl = Normalizer2Factory::getNFCImpl(*status);
    if (U_FAILURE(*status)) {
        return;
    }
    int16_t curClass = nfcImpl->getFCD16(c->tailoringCM) & 0xff;
    CompData *precomp = c->precomp;
    int32_t  compLen = c->compLen;
    UChar *comp = c->comp;
    maxComp = c->precompLen;

    for (int32_t j=0; j < maxComp; j++) {
        int32_t count=0;
        do {
            if ( count == 0 ) {  
                UChar temp[2];
                temp[0]=precomp[j].cp;
                temp[1]=0;
                newDecLen = unorm_normalize(temp, 1, UNORM_NFD, 0,
                            newDecomp, sizeof(newDecomp)/sizeof(UChar), status);
                newDecomp[newDecLen++] = cmLookup->cPoints[c->cmPos];
            }
            else {  
                uprv_memcpy(newDecomp, c->decomp, sizeof(UChar)*(c->decompLen));
                newDecLen = c->decompLen;
                newDecomp[newDecLen++] = precomp[j].cClass;
            }
            newDecomp[newDecLen] = 0;
            compLen = unorm_normalize(newDecomp, newDecLen, UNORM_NFC, 0,
                              comp, 256, status);
            if (compLen==1) {
                comp[compLen++] = newDecomp[newDecLen++] = c->tailoringCM;
                comp[compLen] = newDecomp[newDecLen] = 0;
                el->cPoints = newDecomp;
                el->cSize = newDecLen;

                UCAElements *prefix=(UCAElements *)uhash_get(t->prefixLookup, el);
                el->cPoints = c->comp;
                el->cSize = compLen;
                el->prefix = el->prefixChars;
                el->prefixSize = 0;
                if(prefix == NULL) {
                    el->noOfCEs = 0;
                    ucol_setText(colEl, newDecomp, newDecLen, status);
                    while((el->CEs[el->noOfCEs] = ucol_next(colEl, status)) != (uint32_t)UCOL_NULLORDER) {
                        el->noOfCEs++;
                    }
                    uprv_uca_setMapCE(t, el, status);
                    uprv_uca_finalizeAddition(t, el, status);

                    
                    
                    precomp[c->precompLen].cp=comp[0];
                    precomp[c->precompLen].cClass = curClass;
                    c->precompLen++;
                }
            }
        } while (++count<2 && (precomp[j].cClass == curClass));
    }

}

static void
uprv_uca_addTailCanonicalClosures(tempUCATable *t,
                                  UCollationElements* colEl,
                                  UChar baseCh,
                                  UChar cMark,
                                  UCAElements *el,
                                  UErrorCode *status) {
    CombinClassTable *cmLookup = t->cmLookup;
    const Normalizer2Impl *nfcImpl = Normalizer2Factory::getNFCImpl(*status);
    if (U_FAILURE(*status)) {
        return;
    }
    int16_t maxIndex = nfcImpl->getFCD16(cMark) & 0xff;
    UCAElements element;
    uint16_t *index;
    UChar  decomp[256];
    UChar  comp[256];
    CompData precomp[256];   
    int32_t  precompLen = 0; 
    int32_t i, len, decompLen, replacedPos;
    tempTailorContext c;

    if ( cmLookup == NULL ) {
        return;
    }
    index = cmLookup->index;
    int32_t cClass=nfcImpl->getFCD16(cMark) & 0xff;
    maxIndex = (int32_t)index[(nfcImpl->getFCD16(cMark) & 0xff)-1];
    c.comp = comp;
    c.decomp = decomp;
    c.precomp = precomp;
    c.tailoringCM =  cMark;

    if (cClass>0) {
        maxIndex = (int32_t)index[cClass-1];
    }
    else {
        maxIndex=0;
    }
    decomp[0]=baseCh;
    for ( i=0; i<maxIndex ; i++ ) {
        decomp[1] = cmLookup->cPoints[i];
        decomp[2]=0;
        decompLen=2;
        len = unorm_normalize(decomp, decompLen, UNORM_NFC, 0, comp, 256, status);
        if (len==1) {
            
            
            precomp[precompLen].cp=comp[0];
            precomp[precompLen].cClass =
                       index[nfcImpl->getFCD16(decomp[1]) & 0xff];
            precompLen++;
            replacedPos=0;
            for (decompLen=0; decompLen< (int32_t)el->cSize; decompLen++) {
                decomp[decompLen] = el->cPoints[decompLen];
                if (decomp[decompLen]==cMark) {
                    replacedPos = decompLen;  
                }
            }
            if ( replacedPos != 0 ) {
                decomp[replacedPos]=cmLookup->cPoints[i];
            }
            decomp[decompLen] = 0;
            len = unorm_normalize(decomp, decompLen, UNORM_NFC, 0, comp, 256, status);
            comp[len++] = decomp[decompLen++] = cMark;
            comp[len] = decomp[decompLen] = 0;
            element.cPoints = decomp;
            element.cSize = decompLen;
            element.noOfCEs = 0;
            element.prefix = el->prefixChars;
            element.prefixSize = 0;

            UCAElements *prefix=(UCAElements *)uhash_get(t->prefixLookup, &element);
            element.cPoints = comp;
            element.cSize = len;
            element.prefix = el->prefixChars;
            element.prefixSize = 0;
            if(prefix == NULL) {
                element.noOfCEs = 0;
                ucol_setText(colEl, decomp, decompLen, status);
                while((element.CEs[element.noOfCEs] = ucol_next(colEl, status)) != (uint32_t)UCOL_NULLORDER) {
                    element.noOfCEs++;
                }
                uprv_uca_setMapCE(t, &element, status);
                uprv_uca_finalizeAddition(t, &element, status);
            }

            
            
            if ((len>2) && 
                (nfcImpl->getFCD16(comp[len-2]) & 0xff00)==0) {
                uprv_uca_addFCD4AccentedContractions(t, colEl, comp, len, &element, status);
            }

            if (precompLen >1) {
                c.compLen = len;
                c.decompLen = decompLen;
                c.precompLen = precompLen;
                c.cmPos = i;
                uprv_uca_addMultiCMContractions(t, colEl, &c, &element, status);
                precompLen = c.precompLen;
            }
        }
    }
}

U_CFUNC int32_t U_EXPORT2
uprv_uca_canonicalClosure(tempUCATable *t,
                          UColTokenParser *src,
                          UnicodeSet *closed,
                          UErrorCode *status)
{
    enumStruct context;
    context.closed = closed;
    context.noOfClosures = 0;
    UCAElements el;
    UColToken *tok;
    uint32_t i = 0, j = 0;
    UChar  baseChar, firstCM;
    context.nfcImpl=Normalizer2Factory::getNFCImpl(*status);
    if(U_FAILURE(*status)) {
        return 0;
    }

    UCollator *tempColl = NULL;
    tempUCATable *tempTable = uprv_uca_cloneTempTable(t, status);
    
    if (U_FAILURE(*status)) {
        return 0;
    }

    UCATableHeader *tempData = uprv_uca_assembleTable(tempTable, status);
    tempColl = ucol_initCollator(tempData, 0, t->UCA, status);
    if ( tempTable->cmLookup != NULL ) {
        t->cmLookup = tempTable->cmLookup;  
        tempTable->cmLookup = NULL;
    }
    uprv_uca_closeTempTable(tempTable);

    if(U_SUCCESS(*status)) {
        tempColl->ucaRules = NULL;
        tempColl->actualLocale = NULL;
        tempColl->validLocale = NULL;
        tempColl->requestedLocale = NULL;
        tempColl->hasRealData = TRUE;
        tempColl->freeImageOnClose = TRUE;
    } else if(tempData != 0) {
        uprv_free(tempData);
    }

    
    UCollationElements* colEl = ucol_openElements(tempColl, NULL, 0, status);
    
    if (U_FAILURE(*status)) {
        return 0;
    }
    context.t = t;
    context.tempColl = tempColl;
    context.colEl = colEl;
    context.status = status;
    u_enumCharTypes(_enumCategoryRangeClosureCategory, &context);

    if ( (src==NULL) || !src->buildCCTabFlag ) {
        ucol_closeElements(colEl);
        ucol_close(tempColl);
        return context.noOfClosures;  
    }

    for (i=0; i < src->resultLen; i++) {
        baseChar = firstCM= (UChar)0;
        tok = src->lh[i].first;
        while (tok != NULL && U_SUCCESS(*status)) {
            el.prefix = el.prefixChars;
            el.cPoints = el.uchars;
            if(tok->prefix != 0) {
                el.prefixSize = tok->prefix>>24;
                uprv_memcpy(el.prefix, src->source + (tok->prefix & 0x00FFFFFF), el.prefixSize*sizeof(UChar));

                el.cSize = (tok->source >> 24)-(tok->prefix>>24);
                uprv_memcpy(el.uchars, (tok->source & 0x00FFFFFF)+(tok->prefix>>24) + src->source, el.cSize*sizeof(UChar));
            } else {
                el.prefixSize = 0;
                *el.prefix = 0;

                el.cSize = (tok->source >> 24);
                uprv_memcpy(el.uchars, (tok->source & 0x00FFFFFF) + src->source, el.cSize*sizeof(UChar));
            }
            if(src->UCA != NULL) {
                for(j = 0; j<el.cSize; j++) {
                    int16_t fcd = context.nfcImpl->getFCD16(el.cPoints[j]);
                    if ( (fcd & 0xff) == 0 ) {
                        baseChar = el.cPoints[j];  
                        firstCM=0;  
                    }
                    else {
                        if ( (baseChar!=0) && (firstCM==0) ) {
                            firstCM = el.cPoints[j];  
                        }
                    }
                }
            }
            if ( (baseChar!= (UChar)0) && (firstCM != (UChar)0) ) {
                
                uprv_uca_addTailCanonicalClosures(t, colEl, baseChar, firstCM, &el, status);
            }
            tok = tok->next;
        }
    }
    ucol_closeElements(colEl);
    ucol_close(tempColl);

    return context.noOfClosures;
}

#endif 
