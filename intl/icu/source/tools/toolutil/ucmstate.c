


















#include "unicode/utypes.h"
#include "cstring.h"
#include "cmemory.h"
#include "uarrsort.h"
#include "ucnvmbcs.h"
#include "ucnv_ext.h"
#include "uparse.h"
#include "ucm.h"
#include <stdio.h>

#if !UCONFIG_NO_CONVERSION


















static const char *
parseState(const char *s, int32_t state[256], uint32_t *pFlags) {
    const char *t;
    uint32_t start, end, i;
    int32_t entry;

    
    for(i=0; i<256; ++i) {
        state[i]=MBCS_ENTRY_FINAL(0, MBCS_STATE_ILLEGAL, 0xffff);
    }

    
    s=u_skipWhitespace(s);

    
    if(uprv_strncmp("initial", s, 7)==0) {
        *pFlags=MBCS_STATE_FLAG_DIRECT;
        s=u_skipWhitespace(s+7);
        if(*s++!=',') {
            return s-1;
        }
    } else if(*pFlags==0 && uprv_strncmp("surrogates", s, 10)==0) {
        *pFlags=MBCS_STATE_FLAG_SURROGATES;
        s=u_skipWhitespace(s+10);
        if(*s++!=',') {
            return s-1;
        }
    } else if(*s==0) {
        
        return NULL;
    }

    for(;;) {
        
        s=u_skipWhitespace(s);
        start=uprv_strtoul(s, (char **)&t, 16);
        if(s==t || 0xff<start) {
            return s;
        }
        s=u_skipWhitespace(t);

        
        if(*s=='-') {
            s=u_skipWhitespace(s+1);
            end=uprv_strtoul(s, (char **)&t, 16);
            if(s==t || end<start || 0xff<end) {
                return s;
            }
            s=u_skipWhitespace(t);
        } else {
            end=start;
        }

        
        if(*s!=':' && *s!='.') {
            
            entry=MBCS_ENTRY_FINAL(0, MBCS_STATE_VALID_16, 0);
        } else {
            entry=MBCS_ENTRY_TRANSITION(0, 0);
            if(*s==':') {
                
                s=u_skipWhitespace(s+1);
                i=uprv_strtoul(s, (char **)&t, 16);
                if(s!=t) {
                    if(0x7f<i) {
                        return s;
                    }
                    s=u_skipWhitespace(t);
                    entry=MBCS_ENTRY_SET_STATE(entry, i);
                }
            }

            
            if(*s=='.') {
                
                entry=MBCS_ENTRY_SET_FINAL(entry);

                s=u_skipWhitespace(s+1);
                if(*s=='u') {
                    
                    entry=MBCS_ENTRY_FINAL_SET_ACTION_VALUE(entry, MBCS_STATE_UNASSIGNED, 0xfffe);
                    s=u_skipWhitespace(s+1);
                } else if(*s=='p') {
                    if(*pFlags!=MBCS_STATE_FLAG_DIRECT) {
                        entry=MBCS_ENTRY_FINAL_SET_ACTION(entry, MBCS_STATE_VALID_16_PAIR);
                    } else {
                        entry=MBCS_ENTRY_FINAL_SET_ACTION(entry, MBCS_STATE_VALID_16);
                    }
                    s=u_skipWhitespace(s+1);
                } else if(*s=='s') {
                    entry=MBCS_ENTRY_FINAL_SET_ACTION(entry, MBCS_STATE_CHANGE_ONLY);
                    s=u_skipWhitespace(s+1);
                } else if(*s=='i') {
                    
                    entry=MBCS_ENTRY_FINAL_SET_ACTION_VALUE(entry, MBCS_STATE_ILLEGAL, 0xffff);
                    s=u_skipWhitespace(s+1);
                } else {
                    
                    entry=MBCS_ENTRY_FINAL_SET_ACTION(entry, MBCS_STATE_VALID_16);
                }
            } else {
                
            }
        }

        
        if(MBCS_ENTRY_FINAL_ACTION(entry)==MBCS_STATE_VALID_16) {
            switch(*pFlags) {
            case 0:
                
                break;
            case MBCS_STATE_FLAG_DIRECT:
                
                entry=MBCS_ENTRY_FINAL_SET_ACTION_VALUE(entry, MBCS_STATE_VALID_DIRECT_16, 0xfffe);
                break;
            case MBCS_STATE_FLAG_SURROGATES:
                entry=MBCS_ENTRY_FINAL_SET_ACTION_VALUE(entry, MBCS_STATE_VALID_16_PAIR, 0);
                break;
            default:
                break;
            }
        }

        
        for(i=start; i<=end; ++i) {
            state[i]=entry;
        }

        if(*s==',') {
            ++s;
        } else {
            return *s==0 ? NULL : s;
        }
    }
}

U_CAPI void U_EXPORT2
ucm_addState(UCMStates *states, const char *s) {
    const char *error;

    if(states->countStates==MBCS_MAX_STATE_COUNT) {
        fprintf(stderr, "ucm error: too many states (maximum %u)\n", MBCS_MAX_STATE_COUNT);
        exit(U_INVALID_TABLE_FORMAT);
    }

    error=parseState(s, states->stateTable[states->countStates],
                       &states->stateFlags[states->countStates]);
    if(error!=NULL) {
        fprintf(stderr, "ucm error: parse error in state definition at '%s'\n", error);
        exit(U_INVALID_TABLE_FORMAT);
    }

    ++states->countStates;
}

U_CAPI UBool U_EXPORT2
ucm_parseHeaderLine(UCMFile *ucm,
                    char *line, char **pKey, char **pValue) {
    UCMStates *states;
    char *s, *end;
    char c;

    states=&ucm->states;

    
    for(end=line; (c=*end)!=0; ++end) {
        if(c=='#' || c=='\r' || c=='\n') {
            break;
        }
    }
    while(end>line && (*(end-1)==' ' || *(end-1)=='\t')) {
        --end;
    }
    *end=0;

    
    s=(char *)u_skipWhitespace(line);
    if(*s==0) {
        return TRUE;
    }

    
    if(uprv_memcmp(s, "CHARMAP", 7)==0) {
        return FALSE;
    }

    
    if(*s!='<') {
        fprintf(stderr, "ucm error: no header field <key> in line \"%s\"\n", line);
        exit(U_INVALID_TABLE_FORMAT);
    }
    *pKey=++s;
    while(*s!='>') {
        if(*s==0) {
            fprintf(stderr, "ucm error: incomplete header field <key> in line \"%s\"\n", line);
            exit(U_INVALID_TABLE_FORMAT);
        }
        ++s;
    }
    *s=0;

    
    s=(char *)u_skipWhitespace(s+1);
    if(*s!='"') {
        *pValue=s;
    } else {
        
        *pValue=s+1;
        if(end>*pValue && *(end-1)=='"') {
            *--end=0;
        }
    }

    
    if(uprv_strcmp(*pKey, "uconv_class")==0) {
        if(uprv_strcmp(*pValue, "DBCS")==0) {
            states->conversionType=UCNV_DBCS;
        } else if(uprv_strcmp(*pValue, "SBCS")==0) {
            states->conversionType = UCNV_SBCS;
        } else if(uprv_strcmp(*pValue, "MBCS")==0) {
            states->conversionType = UCNV_MBCS;
        } else if(uprv_strcmp(*pValue, "EBCDIC_STATEFUL")==0) {
            states->conversionType = UCNV_EBCDIC_STATEFUL;
        } else {
            fprintf(stderr, "ucm error: unknown <uconv_class> %s\n", *pValue);
            exit(U_INVALID_TABLE_FORMAT);
        }
        return TRUE;
    } else if(uprv_strcmp(*pKey, "mb_cur_max")==0) {
        c=**pValue;
        if('1'<=c && c<='4' && (*pValue)[1]==0) {
            states->maxCharLength=(int8_t)(c-'0');
            states->outputType=(int8_t)(states->maxCharLength-1);
        } else {
            fprintf(stderr, "ucm error: illegal <mb_cur_max> %s\n", *pValue);
            exit(U_INVALID_TABLE_FORMAT);
        }
        return TRUE;
    } else if(uprv_strcmp(*pKey, "mb_cur_min")==0) {
        c=**pValue;
        if('1'<=c && c<='4' && (*pValue)[1]==0) {
            states->minCharLength=(int8_t)(c-'0');
        } else {
            fprintf(stderr, "ucm error: illegal <mb_cur_min> %s\n", *pValue);
            exit(U_INVALID_TABLE_FORMAT);
        }
        return TRUE;
    } else if(uprv_strcmp(*pKey, "icu:state")==0) {
        
        switch(states->conversionType) {
        case UCNV_SBCS:
        case UCNV_DBCS:
        case UCNV_EBCDIC_STATEFUL:
            states->conversionType=UCNV_MBCS;
            break;
        case UCNV_MBCS:
            break;
        default:
            fprintf(stderr, "ucm error: <icu:state> entry for non-MBCS table or before the <uconv_class> line\n");
            exit(U_INVALID_TABLE_FORMAT);
        }

        if(states->maxCharLength==0) {
            fprintf(stderr, "ucm error: <icu:state> before the <mb_cur_max> line\n");
            exit(U_INVALID_TABLE_FORMAT);
        }
        ucm_addState(states, *pValue);
        return TRUE;
    } else if(uprv_strcmp(*pKey, "icu:base")==0) {
        if(**pValue==0) {
            fprintf(stderr, "ucm error: <icu:base> without a base table name\n");
            exit(U_INVALID_TABLE_FORMAT);
        }
        uprv_strcpy(ucm->baseName, *pValue);
        return TRUE;
    }

    return FALSE;
}



static int32_t
sumUpStates(UCMStates *states) {
    int32_t entry, sum, state, cell, count;
    UBool allStatesReady;

    







    allStatesReady=FALSE;
    for(count=states->countStates; !allStatesReady && count>=0; --count) {
        allStatesReady=TRUE;
        for(state=states->countStates-1; state>=0; --state) {
            if(!(states->stateFlags[state]&MBCS_STATE_FLAG_READY)) {
                allStatesReady=FALSE;
                sum=0;

                
                for(cell=0; cell<256; ++cell) {
                    entry=states->stateTable[state][cell];
                    if(MBCS_ENTRY_IS_FINAL(entry)) {
                        switch(MBCS_ENTRY_FINAL_ACTION(entry)) {
                        case MBCS_STATE_VALID_16:
                            states->stateTable[state][cell]=MBCS_ENTRY_FINAL_SET_VALUE(entry, sum);
                            sum+=1;
                            break;
                        case MBCS_STATE_VALID_16_PAIR:
                            states->stateTable[state][cell]=MBCS_ENTRY_FINAL_SET_VALUE(entry, sum);
                            sum+=2;
                            break;
                        default:
                            
                            break;
                        }
                    }
                }

                
                for(cell=0; cell<256; ++cell) {
                    entry=states->stateTable[state][cell];
                    if(MBCS_ENTRY_IS_TRANSITION(entry)) {
                        if(states->stateFlags[MBCS_ENTRY_TRANSITION_STATE(entry)]&MBCS_STATE_FLAG_READY) {
                            states->stateTable[state][cell]=MBCS_ENTRY_TRANSITION_SET_OFFSET(entry, sum);
                            sum+=states->stateOffsetSum[MBCS_ENTRY_TRANSITION_STATE(entry)];
                        } else {
                            
                            sum=-1;
                            break;
                        }
                    }
                }

                if(sum!=-1) {
                    states->stateOffsetSum[state]=sum;
                    states->stateFlags[state]|=MBCS_STATE_FLAG_READY;
                }
            }
        }
    }

    if(!allStatesReady) {
        fprintf(stderr, "ucm error: the state table contains loops\n");
        exit(U_INVALID_TABLE_FORMAT);
    }

    




    sum=states->stateOffsetSum[0];
    for(state=1; state<states->countStates; ++state) {
        if((states->stateFlags[state]&0xf)==MBCS_STATE_FLAG_DIRECT) {
            int32_t sum2=sum;
            sum+=states->stateOffsetSum[state];
            for(cell=0; cell<256; ++cell) {
                entry=states->stateTable[state][cell];
                if(MBCS_ENTRY_IS_TRANSITION(entry)) {
                    states->stateTable[state][cell]=MBCS_ENTRY_TRANSITION_ADD_OFFSET(entry, sum2);
                }
            }
        }
    }

    
    return states->countToUCodeUnits=(sum+1)&~1;
}

U_CAPI void U_EXPORT2
ucm_processStates(UCMStates *states, UBool ignoreSISOCheck) {
    int32_t entry, state, cell, count;

    if(states->conversionType==UCNV_UNSUPPORTED_CONVERTER) {
        fprintf(stderr, "ucm error: missing conversion type (<uconv_class>)\n");
        exit(U_INVALID_TABLE_FORMAT);
    }

    if(states->countStates==0) {
        switch(states->conversionType) {
        case UCNV_SBCS:
            
            if(states->maxCharLength!=1) {
                fprintf(stderr, "error: SBCS codepage with max B/char!=1\n");
                exit(U_INVALID_TABLE_FORMAT);
            }
            states->conversionType=UCNV_MBCS;
            ucm_addState(states, "0-ff");
            break;
        case UCNV_MBCS:
            fprintf(stderr, "ucm error: missing state table information (<icu:state>) for MBCS\n");
            exit(U_INVALID_TABLE_FORMAT);
            break;
        case UCNV_EBCDIC_STATEFUL:
            
            if(states->minCharLength!=1 || states->maxCharLength!=2) {
                fprintf(stderr, "error: DBCS codepage with min B/char!=1 or max B/char!=2\n");
                exit(U_INVALID_TABLE_FORMAT);
            }
            states->conversionType=UCNV_MBCS;
            ucm_addState(states, "0-ff, e:1.s, f:0.s");
            ucm_addState(states, "initial, 0-3f:4, e:1.s, f:0.s, 40:3, 41-fe:2, ff:4");
            ucm_addState(states, "0-40:1.i, 41-fe:1., ff:1.i");
            ucm_addState(states, "0-ff:1.i, 40:1.");
            ucm_addState(states, "0-ff:1.i");
            break;
        case UCNV_DBCS:
            
            if(states->minCharLength!=2 || states->maxCharLength!=2) {
                fprintf(stderr, "error: DBCS codepage with min or max B/char!=2\n");
                exit(U_INVALID_TABLE_FORMAT);
            }
            states->conversionType = UCNV_MBCS;
            ucm_addState(states, "0-3f:3, 40:2, 41-fe:1, ff:3");
            ucm_addState(states, "41-fe");
            ucm_addState(states, "40");
            ucm_addState(states, "");
            break;
        default:
            fprintf(stderr, "ucm error: unknown charset structure\n");
            exit(U_INVALID_TABLE_FORMAT);
            break;
        }
    }

    





    if(states->maxCharLength<states->minCharLength) {
        fprintf(stderr, "ucm error: max B/char < min B/char\n");
        exit(U_INVALID_TABLE_FORMAT);
    }

    
    count=0;
    for(state=0; state<states->countStates; ++state) {
        if((states->stateFlags[state]&0xf)!=MBCS_STATE_FLAG_DIRECT) {
            ++count;
        }
    }
    if(states->maxCharLength>count+1) {
        fprintf(stderr, "ucm error: max B/char too large\n");
        exit(U_INVALID_TABLE_FORMAT);
    }

    if(states->minCharLength==1) {
        int32_t action;

        



        for(cell=0; cell<256; ++cell) {
            entry=states->stateTable[0][cell];
            if( MBCS_ENTRY_IS_FINAL(entry) &&
                ((action=MBCS_ENTRY_FINAL_ACTION(entry))==MBCS_STATE_VALID_DIRECT_16 ||
                 action==MBCS_STATE_UNASSIGNED)
            ) {
                break;
            }
        }

        if(cell==256) {
            fprintf(stderr, "ucm warning: min B/char too small\n");
        }
    }

    




    for(state=states->countStates-1; state>=0; --state) {
        for(cell=0; cell<256; ++cell) {
            entry=states->stateTable[state][cell];
            if((uint8_t)MBCS_ENTRY_STATE(entry)>=states->countStates) {
                fprintf(stderr, "ucm error: state table entry [%x][%x] has a next state of %x that is too high\n",
                    (int)state, (int)cell, (int)MBCS_ENTRY_STATE(entry));
                exit(U_INVALID_TABLE_FORMAT);
            }
            if(MBCS_ENTRY_IS_FINAL(entry) && (states->stateFlags[MBCS_ENTRY_STATE(entry)]&0xf)!=MBCS_STATE_FLAG_DIRECT) {
                fprintf(stderr, "ucm error: state table entry [%x][%x] is final but has a non-initial next state of %x\n",
                    (int)state, (int)cell, (int)MBCS_ENTRY_STATE(entry));
                exit(U_INVALID_TABLE_FORMAT);
            } else if(MBCS_ENTRY_IS_TRANSITION(entry) && (states->stateFlags[MBCS_ENTRY_STATE(entry)]&0xf)==MBCS_STATE_FLAG_DIRECT) {
                fprintf(stderr, "ucm error: state table entry [%x][%x] is not final but has an initial next state of %x\n",
                    (int)state, (int)cell, (int)MBCS_ENTRY_STATE(entry));
                exit(U_INVALID_TABLE_FORMAT);
            }
        }
    }

    
    if(states->countStates>=2 && (states->stateFlags[1]&0xf)==MBCS_STATE_FLAG_DIRECT) {
        if(states->maxCharLength!=2) {
            fprintf(stderr, "ucm error: SI/SO codepages must have max 2 bytes/char (not %x)\n", (int)states->maxCharLength);
            exit(U_INVALID_TABLE_FORMAT);
        }
        if(states->countStates<3) {
            fprintf(stderr, "ucm error: SI/SO codepages must have at least 3 states (not %x)\n", (int)states->countStates);
            exit(U_INVALID_TABLE_FORMAT);
        }
        
        if( ignoreSISOCheck ||
           (states->stateTable[0][0xe]==MBCS_ENTRY_FINAL(1, MBCS_STATE_CHANGE_ONLY, 0) &&
            states->stateTable[0][0xf]==MBCS_ENTRY_FINAL(0, MBCS_STATE_CHANGE_ONLY, 0) &&
            states->stateTable[1][0xe]==MBCS_ENTRY_FINAL(1, MBCS_STATE_CHANGE_ONLY, 0) &&
            states->stateTable[1][0xf]==MBCS_ENTRY_FINAL(0, MBCS_STATE_CHANGE_ONLY, 0))
        ) {
            states->outputType=MBCS_OUTPUT_2_SISO;
        } else {
            fprintf(stderr, "ucm error: SI/SO codepages must have in states 0 and 1 transitions e:1.s, f:0.s\n");
            exit(U_INVALID_TABLE_FORMAT);
        }
        state=2;
    } else {
        state=1;
    }

    
    while(state<states->countStates) {
        if((states->stateFlags[state]&0xf)==MBCS_STATE_FLAG_DIRECT) {
            fprintf(stderr, "ucm error: state %d is 'initial' - not supported except for SI/SO codepages\n", (int)state);
            exit(U_INVALID_TABLE_FORMAT);
        }
        ++state;
    }

    sumUpStates(states);
}


U_CAPI int32_t U_EXPORT2
ucm_findFallback(_MBCSToUFallback *toUFallbacks, int32_t countToUFallbacks,
                 uint32_t offset) {
    int32_t i;

    if(countToUFallbacks==0) {
        
        return -1;
    }

    
    for(i=0; i<countToUFallbacks; ++i) {
        if(offset==toUFallbacks[i].offset) {
            return i;
        }
    }
    return -1;
}






static void
compactToUnicode2(UCMStates *states,
                  uint16_t **pUnicodeCodeUnits,
                  _MBCSToUFallback *toUFallbacks, int32_t countToUFallbacks,
                  UBool verbose) {
    int32_t (*oldStateTable)[256];
    uint16_t count[256];
    uint16_t *oldUnicodeCodeUnits;
    int32_t entry, offset, oldOffset, trailOffset, oldTrailOffset, savings, sum;
    int32_t i, j, leadState, trailState, newState, fallback;
    uint16_t unit;

    
    if(states->outputType==MBCS_OUTPUT_2_SISO) {
        
        leadState=1;
    } else {
        leadState=0;
    }

    
    uprv_memset(count, 0, sizeof(count));
    for(i=0; i<256; ++i) {
        entry=states->stateTable[leadState][i];
        if(MBCS_ENTRY_IS_TRANSITION(entry)) {
            ++count[MBCS_ENTRY_TRANSITION_STATE(entry)];
        }
    }
    trailState=0;
    for(i=1; i<states->countStates; ++i) {
        if(count[i]>count[trailState]) {
            trailState=i;
        }
    }

    
    uprv_memset(count, 0, sizeof(count));
    savings=0;
    
    for(i=0; i<256; ++i) {
        entry=states->stateTable[leadState][i];
        if(MBCS_ENTRY_IS_TRANSITION(entry) && (MBCS_ENTRY_TRANSITION_STATE(entry))==trailState) {
            
            offset=MBCS_ENTRY_TRANSITION_OFFSET(entry);
            
            for(j=0; j<256; ++j) {
                entry=states->stateTable[trailState][j];
                switch(MBCS_ENTRY_FINAL_ACTION(entry)) {
                case MBCS_STATE_VALID_16:
                    entry=offset+MBCS_ENTRY_FINAL_VALUE_16(entry);
                    if((*pUnicodeCodeUnits)[entry]==0xfffe && ucm_findFallback(toUFallbacks, countToUFallbacks, entry)<0) {
                        ++count[i];
                    } else {
                        j=999; 
                    }
                    break;
                case MBCS_STATE_VALID_16_PAIR:
                    entry=offset+MBCS_ENTRY_FINAL_VALUE_16(entry);
                    if((*pUnicodeCodeUnits)[entry]==0xfffe) {
                        count[i]+=2;
                    } else {
                        j=999; 
                    }
                    break;
                default:
                    break;
                }
            }
            if(j==256) {
                
                savings+=count[i];
            } else {
                count[i]=0;
            }
        }
    }
    
    savings=savings*2-1024; 
    if(savings<=0) {
        return;
    }
    if(verbose) {
        printf("compacting toUnicode data saves %ld bytes\n", (long)savings);
    }
    if(states->countStates>=MBCS_MAX_STATE_COUNT) {
        fprintf(stderr, "cannot compact toUnicode because the maximum number of states is reached\n");
        return;
    }

    
    oldStateTable=(int32_t (*)[256])uprv_malloc(states->countStates*1024);
    if(oldStateTable==NULL) {
        fprintf(stderr, "cannot compact toUnicode: out of memory\n");
        return;
    }
    uprv_memcpy(oldStateTable, states->stateTable, states->countStates*1024);

    
    



    newState=states->countStates++;
    states->stateFlags[newState]=0;
    
    for(i=0; i<256; ++i) {
        entry=states->stateTable[trailState][i];
        switch(MBCS_ENTRY_FINAL_ACTION(entry)) {
        case MBCS_STATE_VALID_16:
        case MBCS_STATE_VALID_16_PAIR:
            states->stateTable[newState][i]=MBCS_ENTRY_FINAL_SET_ACTION_VALUE(entry, MBCS_STATE_UNASSIGNED, 0xfffe);
            break;
        default:
            states->stateTable[newState][i]=entry;
            break;
        }
    }

    
    for(i=0; i<256; ++i) {
        if(count[i]>0) {
            states->stateTable[leadState][i]=MBCS_ENTRY_SET_STATE(states->stateTable[leadState][i], newState);
        }
    }

    
    for(i=0; i<states->countStates; ++i) {
        states->stateFlags[i]&=~MBCS_STATE_FLAG_READY;
    }
    sum=sumUpStates(states);

    
    oldUnicodeCodeUnits=*pUnicodeCodeUnits;
    if(sum==0) {
        *pUnicodeCodeUnits=NULL;
        if(oldUnicodeCodeUnits!=NULL) {
            uprv_free(oldUnicodeCodeUnits);
        }
        uprv_free(oldStateTable);
        return;
    }
    *pUnicodeCodeUnits=(uint16_t *)uprv_malloc(sum*sizeof(uint16_t));
    if(*pUnicodeCodeUnits==NULL) {
        fprintf(stderr, "cannot compact toUnicode: out of memory allocating %ld 16-bit code units\n",
            (long)sum);
        
        *pUnicodeCodeUnits=oldUnicodeCodeUnits;
        --states->countStates;
        uprv_memcpy(states->stateTable, oldStateTable, states->countStates*1024);
        uprv_free(oldStateTable);
        return;
    }
    for(i=0; i<sum; ++i) {
        (*pUnicodeCodeUnits)[i]=0xfffe;
    }

    
    







    
    for(leadState=0; leadState<states->countStates; ++leadState) {
        if((states->stateFlags[leadState]&0xf)==MBCS_STATE_FLAG_DIRECT) {
            
            for(i=0; i<256; ++i) {
                entry=states->stateTable[leadState][i];
                if(MBCS_ENTRY_IS_TRANSITION(entry)) {
                    trailState=(uint8_t)MBCS_ENTRY_TRANSITION_STATE(entry);
                    
                    if(trailState!=newState) {
                        trailOffset=MBCS_ENTRY_TRANSITION_OFFSET(entry);
                        oldTrailOffset=MBCS_ENTRY_TRANSITION_OFFSET(oldStateTable[leadState][i]);
                        
                        for(j=0; j<256; ++j) {
                            entry=states->stateTable[trailState][j];
                            
                            switch(MBCS_ENTRY_FINAL_ACTION(entry)) {
                            case MBCS_STATE_VALID_16:
                                offset=trailOffset+MBCS_ENTRY_FINAL_VALUE_16(entry);
                                
                                oldOffset=oldTrailOffset+MBCS_ENTRY_FINAL_VALUE_16(oldStateTable[trailState][j]);
                                unit=(*pUnicodeCodeUnits)[offset]=oldUnicodeCodeUnits[oldOffset];
                                if(unit==0xfffe && (fallback=ucm_findFallback(toUFallbacks, countToUFallbacks, oldOffset))>=0) {
                                    toUFallbacks[fallback].offset=0x80000000|offset;
                                }
                                break;
                            case MBCS_STATE_VALID_16_PAIR:
                                offset=trailOffset+MBCS_ENTRY_FINAL_VALUE_16(entry);
                                
                                oldOffset=oldTrailOffset+MBCS_ENTRY_FINAL_VALUE_16(oldStateTable[trailState][j]);
                                (*pUnicodeCodeUnits)[offset++]=oldUnicodeCodeUnits[oldOffset++];
                                (*pUnicodeCodeUnits)[offset]=oldUnicodeCodeUnits[oldOffset];
                                break;
                            default:
                                break;
                            }
                        }
                    }
                }
            }
        }
    }

    
    for(i=0; i<countToUFallbacks; ++i) {
        toUFallbacks[i].offset&=0x7fffffff;
    }

    
    uprv_free(oldUnicodeCodeUnits);
    uprv_free(oldStateTable);
}









static int32_t
findUnassigned(UCMStates *states,
               uint16_t *unicodeCodeUnits,
               _MBCSToUFallback *toUFallbacks, int32_t countToUFallbacks,
               int32_t state, int32_t offset, uint32_t b) {
    int32_t i, entry, savings, localSavings, belowSavings;
    UBool haveAssigned;

    localSavings=belowSavings=0;
    haveAssigned=FALSE;
    for(i=0; i<256; ++i) {
        entry=states->stateTable[state][i];
        if(MBCS_ENTRY_IS_TRANSITION(entry)) {
            savings=findUnassigned(states,
                        unicodeCodeUnits,
                        toUFallbacks, countToUFallbacks,
                        MBCS_ENTRY_TRANSITION_STATE(entry),
                        offset+MBCS_ENTRY_TRANSITION_OFFSET(entry),
                        (b<<8)|(uint32_t)i);
            if(savings<0) {
                haveAssigned=TRUE;
            } else if(savings>0) {
                printf("    all-unassigned sequences from prefix 0x%02lx state %ld use %ld bytes\n",
                    (unsigned long)((b<<8)|i), (long)state, (long)savings);
                belowSavings+=savings;
            }
        } else if(!haveAssigned) {
            switch(MBCS_ENTRY_FINAL_ACTION(entry)) {
            case MBCS_STATE_VALID_16:
                entry=offset+MBCS_ENTRY_FINAL_VALUE_16(entry);
                if(unicodeCodeUnits[entry]==0xfffe && ucm_findFallback(toUFallbacks, countToUFallbacks, entry)<0) {
                    localSavings+=2;
                } else {
                    haveAssigned=TRUE;
                }
                break;
            case MBCS_STATE_VALID_16_PAIR:
                entry=offset+MBCS_ENTRY_FINAL_VALUE_16(entry);
                if(unicodeCodeUnits[entry]==0xfffe) {
                    localSavings+=4;
                } else {
                    haveAssigned=TRUE;
                }
                break;
            default:
                break;
            }
        }
    }
    if(haveAssigned) {
        return -1;
    } else {
        return localSavings+belowSavings;
    }
}


static void
compactToUnicodeHelper(UCMStates *states,
                       uint16_t *unicodeCodeUnits,
                       _MBCSToUFallback *toUFallbacks, int32_t countToUFallbacks) {
    int32_t state, savings;

    
    for(state=0; state<states->countStates; ++state) {
        if((states->stateFlags[state]&0xf)==MBCS_STATE_FLAG_DIRECT) {
            savings=findUnassigned(states,
                        unicodeCodeUnits,
                        toUFallbacks, countToUFallbacks,
                        state, 0, 0);
            if(savings>0) {
                printf("    all-unassigned sequences from initial state %ld use %ld bytes\n",
                    (long)state, (long)savings);
            }
        }
    }
}

static int32_t
compareFallbacks(const void *context, const void *fb1, const void *fb2) {
    return ((const _MBCSToUFallback *)fb1)->offset-((const _MBCSToUFallback *)fb2)->offset;
}

U_CAPI void U_EXPORT2
ucm_optimizeStates(UCMStates *states,
                   uint16_t **pUnicodeCodeUnits,
                   _MBCSToUFallback *toUFallbacks, int32_t countToUFallbacks,
                   UBool verbose) {
    UErrorCode errorCode;
    int32_t state, cell, entry;

    
    for(state=0; state<states->countStates; ++state) {
        for(cell=0; cell<256; ++cell) {
            entry=states->stateTable[state][cell];
            




            if(MBCS_ENTRY_SET_STATE(entry, 0)==MBCS_ENTRY_FINAL(0, MBCS_STATE_VALID_DIRECT_16, 0xfffe)) {
                states->stateTable[state][cell]=MBCS_ENTRY_FINAL_SET_ACTION(entry, MBCS_STATE_UNASSIGNED);
            }
        }
    }

    
    if(states->maxCharLength==2) {
        compactToUnicode2(states, pUnicodeCodeUnits, toUFallbacks, countToUFallbacks, verbose);
    } else if(states->maxCharLength>2) {
        if(verbose) {
            compactToUnicodeHelper(states, *pUnicodeCodeUnits, toUFallbacks, countToUFallbacks);
        }
    }

    
    





    if(countToUFallbacks>0) {
        errorCode=U_ZERO_ERROR; 
        uprv_sortArray(toUFallbacks, countToUFallbacks,
                       sizeof(_MBCSToUFallback),
                       compareFallbacks, NULL, FALSE, &errorCode);
    }
}



U_CAPI int32_t U_EXPORT2
ucm_countChars(UCMStates *states,
               const uint8_t *bytes, int32_t length) {
    uint32_t offset;
    int32_t i, entry, count;
    uint8_t state;

    offset=0;
    count=0;
    state=0;

    if(states->countStates==0) {
        fprintf(stderr, "ucm error: there is no state information!\n");
        return -1;
    }

    
    if(length==2 && states->outputType==MBCS_OUTPUT_2_SISO) {
        state=1;
    }

    




    for(i=0; i<length; ++i) {
        entry=states->stateTable[state][bytes[i]];
        if(MBCS_ENTRY_IS_TRANSITION(entry)) {
            state=(uint8_t)MBCS_ENTRY_TRANSITION_STATE(entry);
            offset+=MBCS_ENTRY_TRANSITION_OFFSET(entry);
        } else {
            switch(MBCS_ENTRY_FINAL_ACTION(entry)) {
            case MBCS_STATE_ILLEGAL:
                fprintf(stderr, "ucm error: byte sequence ends in illegal state\n");
                return -1;
            case MBCS_STATE_CHANGE_ONLY:
                fprintf(stderr, "ucm error: byte sequence ends in state-change-only\n");
                return -1;
            case MBCS_STATE_UNASSIGNED:
            case MBCS_STATE_FALLBACK_DIRECT_16:
            case MBCS_STATE_VALID_DIRECT_16:
            case MBCS_STATE_FALLBACK_DIRECT_20:
            case MBCS_STATE_VALID_DIRECT_20:
            case MBCS_STATE_VALID_16:
            case MBCS_STATE_VALID_16_PAIR:
                
                ++count;
                state=(uint8_t)MBCS_ENTRY_FINAL_STATE(entry);
                offset=0;
                break;
            default:
                
                fprintf(stderr, "ucm error: byte sequence reached reserved action code, entry: 0x%02lx\n", (unsigned long)entry);
                return -1;
            }
        }
    }

    if(offset!=0) {
        fprintf(stderr, "ucm error: byte sequence too short, ends in non-final state %u\n", state);
        return -1;
    }

    



    if(count>1 && states->outputType==MBCS_OUTPUT_2_SISO && length!=2*count) {
        fprintf(stderr, "ucm error: SI/SO (like EBCDIC-stateful) result with %d characters does not contain all DBCS\n", (int)count);
        return -1;
    }

    return count;
}
#endif

