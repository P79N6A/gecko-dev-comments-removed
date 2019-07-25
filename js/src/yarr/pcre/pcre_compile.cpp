










































#include "pcre_internal.h"

#include <string.h>
#include "yarr/wtf/ASCIICType.h"
#include "jsvector.h"

using namespace WTF;



#define REQ_UNSET (-2)
#define REQ_NONE  (-1)











#define BRASTACK_SIZE 200






static const short escapes[] = {
     0,      0,      0,      0,      0,      0,      0,      0,   
     0,      0,    ':',    ';',    '<',    '=',    '>',    '?',   
   '@',      0, -ESC_B,      0, -ESC_D,      0,      0,      0,   
     0,      0,      0,      0,      0,      0,      0,      0,   
     0,      0,      0, -ESC_S,      0,      0,      0, -ESC_W,   
     0,      0,      0,    '[',   '\\',    ']',    '^',    '_',   
   '`',      7, -ESC_b,      0, -ESC_d,      0,   '\f',      0,   
     0,      0,      0,      0,      0,      0,   '\n',      0,   
     0,      0,    '\r', -ESC_s,   '\t',      0,  '\v', -ESC_w,   
     0,      0,      0                                            
};
static const unsigned OPCODE_LEN = 1;
static const unsigned BRAZERO_LEN = OPCODE_LEN;
static const unsigned BRA_NEST_SIZE = 2;
static const unsigned BRA_LEN = OPCODE_LEN + LINK_SIZE + BRA_NEST_SIZE;
static const unsigned KET_LEN = OPCODE_LEN + LINK_SIZE;




enum ErrorCode {
    ERR0, ERR1, ERR2, ERR3, ERR4, ERR5, ERR6, ERR7, ERR8, ERR9,
    ERR10, ERR11, ERR12, ERR13, ERR14, ERR15, ERR16, ERR17
};



























struct CompileData {
    CompileData() {
        topBackref = 0;
        backrefMap = 0;
        reqVaryOpt = 0;
        needOuterBracket = false;
        numCapturingBrackets = 0;
    }
    int topBackref;            
    unsigned backrefMap;       
    int reqVaryOpt;            
    bool needOuterBracket;
    int numCapturingBrackets;
};



static bool compileBracket(int, int*, unsigned char**, const UChar**, const UChar*, ErrorCode*, int, int*, int*, CompileData&);
static bool bracketIsAnchored(const unsigned char* code);
static bool bracketNeedsLineStart(const unsigned char* code, unsigned captureMap, unsigned backrefMap);
static int bracketFindFirstAssertedCharacter(const unsigned char* code, bool inassert);























static int checkEscape(const UChar** ptrPtr, const UChar* patternEnd, ErrorCode* errorCodePtr, int bracount, bool isClass)
{
    const UChar* ptr = *ptrPtr + 1;

    
    if (ptr == patternEnd) {
        *errorCodePtr = ERR1;
        *ptrPtr = ptr;
        return 0;
    }
    
    int c = *ptr;
    
    


    
    if (c < '0' || c > 'z') { 
    } else if (int escapeValue = escapes[c - '0']) {
        c = escapeValue;
        if (isClass) {
            if (-c == ESC_b)
                c = '\b'; 
            else if (-c == ESC_B)
                c = 'B'; 
        }
    
    
    } else {
        switch (c) {
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                



                
                if (!isClass) {
                    const UChar* oldptr = ptr;
                    c -= '0';
                    while ((ptr + 1 < patternEnd) && isASCIIDigit(ptr[1]) && c <= bracount)
                        c = c * 10 + *(++ptr) - '0';
                    if (c <= bracount) {
                        c = -(ESC_REF + c);
                        break;
                    }
                    ptr = oldptr;      
                }
                
                

                
                if ((c = *ptr) >= '8') {
                    c = '\\';
                    ptr -= 1;
                    break;
                }

            


            case '0': {
                c -= '0';
                int i;
                for (i = 1; i <= 2; ++i) {
                    if (ptr + i >= patternEnd || ptr[i] < '0' || ptr[i] > '7')
                        break;
                    int cc = c * 8 + ptr[i] - '0';
                    if (cc > 255)
                        break;
                    c = cc;
                }
                ptr += i - 1;
                break;
            }

            case 'x': {
                c = 0;
                int i;
                for (i = 1; i <= 2; ++i) {
                    if (ptr + i >= patternEnd || !isASCIIHexDigit(ptr[i])) {
                        c = 'x';
                        i = 1;
                        break;
                    }
                    int cc = ptr[i];
                    if (cc >= 'a')
                        cc -= 32;             
                    c = c * 16 + cc - ((cc < 'A') ? '0' : ('A' - 10));
                }
                ptr += i - 1;
                break;
            }

            case 'u': {
                c = 0;
                int i;
                for (i = 1; i <= 4; ++i) {
                    if (ptr + i >= patternEnd || !isASCIIHexDigit(ptr[i])) {
                        c = 'u';
                        i = 1;
                        break;
                    }
                    int cc = ptr[i];
                    if (cc >= 'a')
                        cc -= 32;             
                    c = c * 16 + cc - ((cc < 'A') ? '0' : ('A' - 10));
                }
                ptr += i - 1;
                break;
            }

            case 'c':
                if (++ptr == patternEnd) {
                    *errorCodePtr = ERR2;
                    return 0;
                }
                
                c = *ptr;

                

                if ((!isClass && !isASCIIAlpha(c)) || (!isASCIIAlphanumeric(c) && c != '_')) {
                    c = '\\';
                    ptr -= 2;
                    break;
                }

                

                c = toASCIIUpper(c) ^ 0x40;
                break;
            }
    }
    
    *ptrPtr = ptr;
    return c;
}
















static bool isCountedRepeat(const UChar* p, const UChar* patternEnd)
{
    if (p >= patternEnd || !isASCIIDigit(*p))
        return false;
    p++;
    while (p < patternEnd && isASCIIDigit(*p))
        p++;
    if (p < patternEnd && *p == '}')
        return true;
    
    if (p >= patternEnd || *p++ != ',')
        return false;
    if (p < patternEnd && *p == '}')
        return true;
    
    if (p >= patternEnd || !isASCIIDigit(*p))
        return false;
    p++;
    while (p < patternEnd && isASCIIDigit(*p))
        p++;
    
    return (p < patternEnd && *p == '}');
}




















static const UChar* readRepeatCounts(const UChar* p, int* minp, int* maxp, ErrorCode* errorCodePtr)
{
    int min = 0;
    int max = -1;
    
    

    
    while (isASCIIDigit(*p))
        min = min * 10 + *p++ - '0';
    if (min < 0 || min > 65535) {
        *errorCodePtr = ERR5;
        return p;
    }
    
    

    
    if (*p == '}')
        max = min;
    else {
        if (*(++p) != '}') {
            max = 0;
            while (isASCIIDigit(*p))
                max = max * 10 + *p++ - '0';
            if (max < 0 || max > 65535) {
                *errorCodePtr = ERR5;
                return p;
            }
            if (max < min) {
                *errorCodePtr = ERR4;
                return p;
            }
        }
    }
    
    

    
    *minp = min;
    *maxp = max;
    return p;
}














static const unsigned char* firstSignificantOpcode(const unsigned char* code)
{
    while (*code == OP_BRANUMBER)
        code += 3;
    return code;
}

static const unsigned char* firstSignificantOpcodeSkippingAssertions(const unsigned char* code)
{
    while (true) {
        switch (*code) {
            case OP_ASSERT_NOT:
                advanceToEndOfBracket(code);
                code += 1 + LINK_SIZE;
                break;
            case OP_WORD_BOUNDARY:
            case OP_NOT_WORD_BOUNDARY:
                ++code;
                break;
            case OP_BRANUMBER:
                code += 3;
                break;
            default:
                return code;
        }
    }
}



















static bool getOthercaseRange(int* cptr, int d, int* ocptr, int* odptr)
{
    int c, othercase = 0;
    
    for (c = *cptr; c <= d; c++) {
        if ((othercase = jsc_pcre_ucp_othercase(c)) >= 0)
            break;
    }
    
    if (c > d)
        return false;
    
    *ocptr = othercase;
    int next = othercase + 1;
    
    for (++c; c <= d; c++) {
        if (jsc_pcre_ucp_othercase(c) != next)
            break;
        next++;
    }
    
    *odptr = next - 1;
    *cptr = c;
    
    return true;
}















static int encodeUTF8(int cvalue, unsigned char *buffer)
{
    int i;
    for (i = 0; i < jsc_pcre_utf8_table1_size; i++)
        if (cvalue <= jsc_pcre_utf8_table1[i])
            break;
    buffer += i;
    for (int j = i; j > 0; j--) {
        *buffer-- = 0x80 | (cvalue & 0x3f);
        cvalue >>= 6;
    }
    *buffer = jsc_pcre_utf8_table2[i] | cvalue;
    return i + 1;
}





















static inline bool safelyCheckNextChar(const UChar* ptr, const UChar* patternEnd, UChar expected)
{
    return ((ptr + 1 < patternEnd) && ptr[1] == expected);
}

static bool
compileBranch(int options, int* brackets, unsigned char** codePtr,
               const UChar** ptrPtr, const UChar* patternEnd, ErrorCode* errorCodePtr, int *firstbyteptr,
               int* reqbyteptr, CompileData& cd)
{
    int repeatType, opType;
    int repeatMin = 0, repeat_max = 0;      
    int bravalue = 0;
    int reqvary, tempreqvary;
    int c;
    unsigned char* code = *codePtr;
    unsigned char* tempcode;
    bool didGroupSetFirstByte = false;
    const UChar* ptr = *ptrPtr;
    const UChar* tempptr;
    unsigned char* previous = NULL;
    unsigned char classbits[32];
    
    bool class_utf8;
    unsigned char* class_utf8data;
    unsigned char utf8_char[6];
    
    








    
    int firstByte = REQ_UNSET;
    int reqByte = REQ_UNSET;
    int zeroReqByte = REQ_UNSET;
    int zeroFirstByte = REQ_UNSET;
    
    



    
    int reqCaseOpt = (options & IgnoreCaseOption) ? REQ_IGNORE_CASE : 0;
    
    
    
    for (;; ptr++) {
        bool negateClass;
        bool shouldFlipNegation; 
        int classCharCount;
        int classLastChar;
        int skipBytes;
        int subReqByte;
        int subFirstByte;
        int mcLength;
        unsigned char mcbuffer[8];
        
        
        
        c = ptr < patternEnd ? *ptr : 0;
        
        

        
        bool isQuantifier = c == '*' || c == '+' || c == '?' || (c == '{' && isCountedRepeat(ptr + 1, patternEnd));
        
        switch (c) {
            
                
            case 0:
                if (ptr < patternEnd)
                    goto NORMAL_CHAR;
                
            case '|':
            case ')':
                *firstbyteptr = firstByte;
                *reqbyteptr = reqByte;
                *codePtr = code;
                *ptrPtr = ptr;
                return true;
                
            


            case '^':
                if (options & MatchAcrossMultipleLinesOption) {
                    if (firstByte == REQ_UNSET)
                        firstByte = REQ_NONE;
                    *code++ = OP_BOL;
                } else
                    *code++ = OP_CIRC;
                previous = NULL;
                break;

            case '$':
                previous = NULL;
                if (options & MatchAcrossMultipleLinesOption)
                  *code++ = OP_EOL;
                else
                  *code++ = OP_DOLL;
                break;

            


            case '.':
                if (firstByte == REQ_UNSET)
                    firstByte = REQ_NONE;
                zeroFirstByte = firstByte;
                zeroReqByte = reqByte;
                previous = code;
                *code++ = OP_NOT_NEWLINE;
                break;
                
            










                
            case '[': {
                previous = code;
                shouldFlipNegation = false;
                
                

                
                

                if (ptr + 1 >= patternEnd) {
                    *errorCodePtr = ERR6;
                    return false;
                }

                if (ptr[1] == '^') {
                    negateClass = true;
                    ++ptr;
                } else
                    negateClass = false;
                
                


                
                classCharCount = 0;
                classLastChar = -1;
                
                class_utf8 = false;                       
                class_utf8data = code + LINK_SIZE + 34;   
                
                



                
                memset(classbits, 0, 32 * sizeof(unsigned char));
                
                




                while ((++ptr < patternEnd) && (c = *ptr) != ']') {
                    






                    
                    if (c == '\\') {
                        c = checkEscape(&ptr, patternEnd, errorCodePtr, cd.numCapturingBrackets, true);
                        if (c < 0) {
                            classCharCount += 2;     
                            switch (-c) {
                                case ESC_d:
                                    for (c = 0; c < 32; c++)
                                        classbits[c] |= classBitmapForChar(c + cbit_digit);
                                    continue;
                                    
                                case ESC_D:
                                    shouldFlipNegation = true;
                                    for (c = 0; c < 32; c++)
                                        classbits[c] |= ~classBitmapForChar(c + cbit_digit);
                                    continue;
                                    
                                case ESC_w:
                                    for (c = 0; c < 32; c++)
                                        classbits[c] |= classBitmapForChar(c + cbit_word);
                                    continue;
                                    
                                case ESC_W:
                                    shouldFlipNegation = true;
                                    for (c = 0; c < 32; c++)
                                        classbits[c] |= ~classBitmapForChar(c + cbit_word);
                                    continue;
                                    
                                case ESC_s:
                                    for (c = 0; c < 32; c++)
                                         classbits[c] |= classBitmapForChar(c + cbit_space);
                                    continue;
                                    
                                case ESC_S:
                                    shouldFlipNegation = true;
                                    for (c = 0; c < 32; c++)
                                         classbits[c] |= ~classBitmapForChar(c + cbit_space);
                                    continue;
                                    
                                    


                                    
                                default:
                                    c = *ptr;              
                                    classCharCount -= 2;  
                            }
                        }
                        
                        

                        
                    }   
                    
                    


                    
                    if ((ptr + 2 < patternEnd) && ptr[1] == '-' && ptr[2] != ']') {
                        ptr += 2;
                        
                        int d = *ptr;
                        
                        


                        
                        if (d == '\\') {
                            const UChar* oldptr = ptr;
                            d = checkEscape(&ptr, patternEnd, errorCodePtr, cd.numCapturingBrackets, true);
                            
                            
                            if (d < 0) {
                                ptr = oldptr - 2;
                                goto LONE_SINGLE_CHARACTER;  
                            }
                        }
                        
                        

                        
                        if (d == c)
                            goto LONE_SINGLE_CHARACTER;  
                        
                        



                        
                        if ((d > 255 || ((options & IgnoreCaseOption) && d > 127))) {
                            class_utf8 = true;
                            
                            


                            
                            if (options & IgnoreCaseOption) {
                                int occ, ocd;
                                int cc = c;
                                int origd = d;
                                while (getOthercaseRange(&cc, origd, &occ, &ocd)) {
                                    if (occ >= c && ocd <= d)
                                        continue;  
                                    
                                    if (occ < c  && ocd >= c - 1)        
                                    {                                  
                                        c = occ;                           
                                        continue;                          
                                    }                                  
                                    if (ocd > d && occ <= d + 1)         
                                    {                                  
                                        d = ocd;
                                        continue;
                                    }
                                    
                                    if (occ == ocd)
                                        *class_utf8data++ = XCL_SINGLE;
                                    else {
                                        *class_utf8data++ = XCL_RANGE;
                                        class_utf8data += encodeUTF8(occ, class_utf8data);
                                    }
                                    class_utf8data += encodeUTF8(ocd, class_utf8data);
                                }
                            }
                            
                            

                            
                            *class_utf8data++ = XCL_RANGE;
                            class_utf8data += encodeUTF8(c, class_utf8data);
                            class_utf8data += encodeUTF8(d, class_utf8data);
                            
                            


                            
                            continue;    
                        }
                        
                        


                        
                        for (; c <= d; c++) {
                            classbits[c/8] |= (1 << (c&7));
                            if (options & IgnoreCaseOption) {
                                int uc = flipCase(c);
                                classbits[uc/8] |= (1 << (uc&7));
                            }
                            classCharCount++;                
                            classLastChar = c;
                        }
                        
                        continue;   
                    }
                    
                    


                    
                LONE_SINGLE_CHARACTER:
                    
                    
                    
                    if ((c > 255 || ((options & IgnoreCaseOption) && c > 127))) {
                        class_utf8 = true;
                        *class_utf8data++ = XCL_SINGLE;
                        class_utf8data += encodeUTF8(c, class_utf8data);
                        
                        if (options & IgnoreCaseOption) {
                            int othercase;
                            if ((othercase = jsc_pcre_ucp_othercase(c)) >= 0) {
                                *class_utf8data++ = XCL_SINGLE;
                                class_utf8data += encodeUTF8(othercase, class_utf8data);
                            }
                        }
                    } else {
                        
                        classbits[c/8] |= (1 << (c&7));
                        if (options & IgnoreCaseOption) {
                            c = flipCase(c);
                            classbits[c/8] |= (1 << (c&7));
                        }
                        classCharCount++;
                        classLastChar = c;
                    }
                }
                
                












                
                if (classCharCount == 1 && (!class_utf8 && (!negateClass || classLastChar < 128))) {
                    zeroReqByte = reqByte;
                    
                    
                    
                    if (negateClass) {
                        if (firstByte == REQ_UNSET)
                            firstByte = REQ_NONE;
                        zeroFirstByte = firstByte;
                        *code++ = OP_NOT;
                        *code++ = classLastChar;
                        break;
                    }
                    
                    

                    
                    c = classLastChar;
                    goto NORMAL_CHAR;
                }       
                
                



                
                if (firstByte == REQ_UNSET) firstByte = REQ_NONE;
                zeroFirstByte = firstByte;
                zeroReqByte = reqByte;
                
                


                
                if (class_utf8 && !shouldFlipNegation) {
                    *class_utf8data++ = XCL_END;    
                    *code++ = OP_XCLASS;
                    code += LINK_SIZE;
                    *code = negateClass? XCL_NOT : 0;
                    
                    

                    
                    if (classCharCount > 0) {
                        *code++ |= XCL_MAP;
                        memcpy(code, classbits, 32);
                        code = class_utf8data;
                    }
                    
                    
                    
                    else {
                        int len = class_utf8data - (code + 33);
                        memmove(code + 1, code + 33, len);
                        code += len + 1;
                    }
                    
                    
                    
                    putLinkValue(previous + 1, code - previous);
                    break;   
                }
                
                



                
                *code++ = (negateClass == shouldFlipNegation) ? OP_CLASS : OP_NCLASS;
                if (negateClass)
                    for (c = 0; c < 32; c++)
                        code[c] = ~classbits[c];
                else
                    memcpy(code, classbits, 32);
                code += 32;
                break;
            }
                
            


            case '{':
                if (!isQuantifier)
                    goto NORMAL_CHAR;
                ptr = readRepeatCounts(ptr + 1, &repeatMin, &repeat_max, errorCodePtr);
                if (*errorCodePtr)
                    goto FAILED;
                goto REPEAT;
                
            case '*':
                repeatMin = 0;
                repeat_max = -1;
                goto REPEAT;
                
            case '+':
                repeatMin = 1;
                repeat_max = -1;
                goto REPEAT;
                
            case '?':
                repeatMin = 0;
                repeat_max = 1;
                
            REPEAT:
                if (!previous) {
                    *errorCodePtr = ERR9;
                    goto FAILED;
                }
                
                if (repeatMin == 0) {
                    firstByte = zeroFirstByte;    
                    reqByte = zeroReqByte;        
                }
                
                
                
                reqvary = (repeatMin == repeat_max) ? 0 : REQ_VARY;
                
                opType = 0;                    
                
                

                
                
                tempcode = previous;
                
                




                
                if (safelyCheckNextChar(ptr, patternEnd, '?')) {
                    repeatType = 1;
                    ptr++;
                } else
                    repeatType = 0;
                
                




                
                if (*previous == OP_CHAR || *previous == OP_CHAR_IGNORING_CASE) {
                    



                    
                    if (code[-1] & 0x80) {
                        unsigned char *lastchar = code - 1;
                        while((*lastchar & 0xc0) == 0x80)
                            lastchar--;
                        c = code - lastchar;            
                        memcpy(utf8_char, lastchar, c); 
                        c |= 0x80;                      
                    }
                    else {
                        c = code[-1];
                        if (repeatMin > 1)
                            reqByte = c | reqCaseOpt | cd.reqVaryOpt;
                    }
                    
                    goto OUTPUT_SINGLE_REPEAT;   
                }
                
                else if (*previous == OP_ASCII_CHAR || *previous == OP_ASCII_LETTER_IGNORING_CASE) {
                    c = previous[1];
                    if (repeatMin > 1)
                        reqByte = c | reqCaseOpt | cd.reqVaryOpt;
                    goto OUTPUT_SINGLE_REPEAT;
                }
                
                



                
                else if (*previous == OP_NOT) {
                    opType = OP_NOTSTAR - OP_STAR;  
                    c = previous[1];
                    goto OUTPUT_SINGLE_REPEAT;
                }
                
                


                
                else if (*previous <= OP_NOT_NEWLINE) {
                    opType = OP_TYPESTAR - OP_STAR;  
                    c = *previous;
                    
                OUTPUT_SINGLE_REPEAT:
                    int prop_type = -1;
                    int prop_value = -1;
                    
                    unsigned char* oldcode = code;
                    code = previous;                  
                    
                    

                    
                    if (repeat_max == 0)
                        goto END_REPEAT;
                    
                    
                    
                    repeatType += opType;
                    
                    

                    
                    if (repeatMin == 0) {
                        if (repeat_max == -1)
                            *code++ = OP_STAR + repeatType;
                        else if (repeat_max == 1)
                            *code++ = OP_QUERY + repeatType;
                        else {
                            *code++ = OP_UPTO + repeatType;
                            put2ByteValueAndAdvance(code, repeat_max);
                        }
                    }
                    
                    



                    
                    else if (repeatMin == 1) {
                        if (repeat_max == -1)
                            *code++ = OP_PLUS + repeatType;
                        else {
                            code = oldcode;                 
                            if (repeat_max == 1)
                                goto END_REPEAT;
                            *code++ = OP_UPTO + repeatType;
                            put2ByteValueAndAdvance(code, repeat_max - 1);
                        }
                    }
                    
                    

                    
                    else {
                        *code++ = OP_EXACT + opType;  
                        put2ByteValueAndAdvance(code, repeatMin);
                        
                        




                        
                        if (repeat_max < 0) {
                            if (c >= 128) {
                                memcpy(code, utf8_char, c & 7);
                                code += c & 7;
                            } else {
                                *code++ = c;
                                if (prop_type >= 0) {
                                    *code++ = prop_type;
                                    *code++ = prop_value;
                                }
                            }
                            *code++ = OP_STAR + repeatType;
                        }
                        
                        

                        
                        else if (repeat_max != repeatMin) {
                            if (c >= 128) {
                                memcpy(code, utf8_char, c & 7);
                                code += c & 7;
                            } else
                                *code++ = c;
                            if (prop_type >= 0) {
                                *code++ = prop_type;
                                *code++ = prop_value;
                            }
                            repeat_max -= repeatMin;
                            *code++ = OP_UPTO + repeatType;
                            put2ByteValueAndAdvance(code, repeat_max);
                        }
                    }
                    
                    
                    
                    if (c >= 128) {
                        memcpy(code, utf8_char, c & 7);
                        code += c & 7;
                    } else
                        *code++ = c;
                    
                    

                    
                    if (prop_type >= 0) {
                        *code++ = prop_type;
                        *code++ = prop_value;
                    }
                }
                
                

                
                else if (*previous == OP_CLASS ||
                         *previous == OP_NCLASS ||
                         *previous == OP_XCLASS ||
                         *previous == OP_REF)
                {
                    if (repeat_max == 0) {
                        code = previous;
                        goto END_REPEAT;
                    }
                    
                    if (repeatMin == 0 && repeat_max == -1)
                        *code++ = OP_CRSTAR + repeatType;
                    else if (repeatMin == 1 && repeat_max == -1)
                        *code++ = OP_CRPLUS + repeatType;
                    else if (repeatMin == 0 && repeat_max == 1)
                        *code++ = OP_CRQUERY + repeatType;
                    else {
                        *code++ = OP_CRRANGE + repeatType;
                        put2ByteValueAndAdvance(code, repeatMin);
                        if (repeat_max == -1)
                            repeat_max = 0;  
                        put2ByteValueAndAdvance(code, repeat_max);
                    }
                }
                
                

                
                else if (*previous >= OP_BRA) {
                    int ketoffset = 0;
                    int len = code - previous;
                    unsigned char* bralink = NULL;
                    int nested = get2ByteValue(previous + 1 + LINK_SIZE);
                    
                    




                    
                    if (repeat_max == -1) {
                        const unsigned char* ket = previous;
                        advanceToEndOfBracket(ket);
                        ketoffset = code - ket;
                    }
                    
                    





                    
                    if (repeatMin == 0) {
                        

                        
                        if (repeat_max == 0) {
                            code = previous;
                            goto END_REPEAT;
                        }
                        
                        




                        
                        if (repeat_max <= 1) {
                            *code = OP_END;
                            memmove(previous+1, previous, len);
                            code++;
                            *previous++ = OP_BRAZERO + repeatType;
                        }
                        
                        





                        
                        else {
                            *code = OP_END;
                            memmove(previous + 4 + LINK_SIZE, previous, len);
                            code += 4 + LINK_SIZE;
                            *previous++ = OP_BRAZERO + repeatType;
                            *previous++ = OP_BRA;
                            
                            

                            
                            int offset = (!bralink) ? 0 : previous - bralink;
                            bralink = previous;
                            putLinkValueAllowZeroAndAdvance(previous, offset);
                            put2ByteValueAndAdvance(previous, nested);
                        }
                        
                        repeat_max--;
                    }
                    
                    



                    
                    else {
                        if (repeatMin > 1) {
                            if (didGroupSetFirstByte && reqByte < 0)
                                reqByte = firstByte;
                            for (int i = 1; i < repeatMin; i++) {
                                memcpy(code, previous, len);
                                code += len;
                            }
                        }
                        if (repeat_max > 0)
                            repeat_max -= repeatMin;
                    }
                    
                    




                    
                    if (repeat_max >= 0) {
                        for (int i = repeat_max - 1; i >= 0; i--) {
                            *code++ = OP_BRAZERO + repeatType;
                            
                            

                            
                            if (i != 0) {
                                *code++ = OP_BRA;
                                int offset = (!bralink) ? 0 : code - bralink;
                                bralink = code;
                                putLinkValueAllowZeroAndAdvance(code, offset);
                                put2ByteValueAndAdvance(code, nested);
                            }
                            
                            memcpy(code, previous, len);
                            code += len;
                        }
                        
                        

                        
                        while (bralink) {
                            int offset = code - bralink + 1;
                            unsigned char* bra = code - offset;
                            int oldlinkoffset = getLinkValueAllowZero(bra + 1);
                            bralink = (!oldlinkoffset) ? 0 : bralink - oldlinkoffset;
                            *code++ = OP_KET;
                            putLinkValueAndAdvance(code, offset);
                            putLinkValue(bra + 1, offset);
                        }
                    }
                    
                    



                    
                    else
                        code[-ketoffset] = OP_KETRMAX + repeatType;
                }
                
                
                
                else if (*previous == OP_ASSERT || *previous == OP_ASSERT_NOT) {
                    if (repeatMin == 0) {
                        code = previous;
                        goto END_REPEAT;
                    }
                }
                
                
                
                else {
                    *errorCodePtr = ERR11;
                    goto FAILED;
                }
                
                


                
            END_REPEAT:
                previous = NULL;
                cd.reqVaryOpt |= reqvary;
                break;
                
            





                
            case '(':
            {
                skipBytes = 2;
                unsigned minBracket = *brackets + 1;
                if (*(++ptr) == '?') {
                    switch (*(++ptr)) {
                        case ':':                 
                            bravalue = OP_BRA;
                            ptr++;
                            break;
                            
                        case '=':                 
                            bravalue = OP_ASSERT;
                            ptr++;
                            break;
                            
                        case '!':                 
                            bravalue = OP_ASSERT_NOT;
                            ptr++;
                            break;
                            
                        
                            
                        default:
                            *errorCodePtr = ERR12;
                            goto FAILED;
                        }
                }
                
                


                
                else {
                    if (++(*brackets) > EXTRACT_BASIC_MAX) {
                        bravalue = OP_BRA + EXTRACT_BASIC_MAX + 1;
                        code[3 + LINK_SIZE] = OP_BRANUMBER;
                        put2ByteValue(code + 4 + LINK_SIZE, *brackets);
                        skipBytes = 5;
                    }
                    else
                        bravalue = OP_BRA + *brackets;
                }
                
                



                
                previous = code;
                *code = bravalue;
                tempcode = code;
                tempreqvary = cd.reqVaryOpt;     
                {
                    unsigned bracketsBeforeRecursion = *brackets;
                    if (!compileBracket(
                                       options,
                                       brackets,                     
                                       &tempcode,                    
                                       &ptr,                         
                                       patternEnd,
                                       errorCodePtr,                 
                                       skipBytes,                    
                                       &subFirstByte,                
                                       &subReqByte,                  
                                       cd))                          
                        goto FAILED;
                    unsigned enclosedBrackets = (*brackets - bracketsBeforeRecursion);
                    unsigned limitBracket = minBracket + enclosedBrackets + (bravalue > OP_BRA);
                    if (!((minBracket & 0xff) == minBracket && (limitBracket & 0xff) == limitBracket)) {
                        *errorCodePtr = ERR17;
                        return false;
                    }
                    JS_ASSERT(minBracket <= limitBracket);
                    put2ByteValue(code + 1 + LINK_SIZE, minBracket << 8 | limitBracket);
                }
                
                



                
                




                
                zeroReqByte = reqByte;
                zeroFirstByte = firstByte;
                didGroupSetFirstByte = false;
                
                if (bravalue >= OP_BRA) {
                    




                    
                    if (firstByte == REQ_UNSET) {
                        if (subFirstByte >= 0) {
                            firstByte = subFirstByte;
                            didGroupSetFirstByte = true;
                        }
                        else
                            firstByte = REQ_NONE;
                        zeroFirstByte = REQ_NONE;
                    }
                    
                    


                    
                    else if (subFirstByte >= 0 && subReqByte < 0)
                        subReqByte = subFirstByte | tempreqvary;
                    
                    

                    
                    if (subReqByte >= 0)
                        reqByte = subReqByte;
                }
                
                






                
                else if (bravalue == OP_ASSERT && subReqByte >= 0)
                    reqByte = subReqByte;
                
                
                
                code = tempcode;
                
                
                
                if (ptr >= patternEnd || *ptr != ')') {
                    *errorCodePtr = ERR14;
                    goto FAILED;
                }
                break;
                
            }
            


                
            case '\\':
                tempptr = ptr;
                c = checkEscape(&ptr, patternEnd, errorCodePtr, cd.numCapturingBrackets, false);
                
                





                
                if (c < 0) {
                    

                    
                    if (firstByte == REQ_UNSET && -c > ESC_b && -c <= ESC_w)
                        firstByte = REQ_NONE;
                    
                    
                    
                    zeroFirstByte = firstByte;
                    zeroReqByte = reqByte;
                    
                    
                    
                    if (-c >= ESC_REF) {
                        int number = -c - ESC_REF;
                        previous = code;
                        *code++ = OP_REF;
                        put2ByteValueAndAdvance(code, number);
                    }
                    
                    

                    
                    else {
                        previous = (-c > ESC_b && -c <= ESC_w) ? code : NULL;
                        *code++ = -c;
                    }
                    continue;
                }
                
                
                
                


                
                default:
            NORMAL_CHAR:
                
                previous = code;
                
                if (c < 128) {
                    mcLength = 1;
                    mcbuffer[0] = c;
                    
                    if ((options & IgnoreCaseOption) && (c | 0x20) >= 'a' && (c | 0x20) <= 'z') {
                        *code++ = OP_ASCII_LETTER_IGNORING_CASE;
                        *code++ = c | 0x20;
                    } else {
                        *code++ = OP_ASCII_CHAR;
                        *code++ = c;
                    }
                } else {
                    mcLength = encodeUTF8(c, mcbuffer);
                    
                    *code++ = (options & IgnoreCaseOption) ? OP_CHAR_IGNORING_CASE : OP_CHAR;
                    for (c = 0; c < mcLength; c++)
                        *code++ = mcbuffer[c];
                }
                
                



                
                if (firstByte == REQ_UNSET) {
                    zeroFirstByte = REQ_NONE;
                    zeroReqByte = reqByte;
                    
                    

                    
                    if (mcLength == 1 || reqCaseOpt == 0) {
                        firstByte = mcbuffer[0] | reqCaseOpt;
                        if (mcLength != 1)
                            reqByte = code[-1] | cd.reqVaryOpt;
                    }
                    else
                        firstByte = reqByte = REQ_NONE;
                }
                
                

                
                else {
                    zeroFirstByte = firstByte;
                    zeroReqByte = reqByte;
                    if (mcLength == 1 || reqCaseOpt == 0)
                        reqByte = code[-1] | reqCaseOpt | cd.reqVaryOpt;
                }
                
                break;            
        }
    }                   
    
    


    
FAILED:
    *ptrPtr = ptr;
    return false;
}



























static bool
compileBracket(int options, int* brackets, unsigned char** codePtr,
    const UChar** ptrPtr, const UChar* patternEnd, ErrorCode* errorCodePtr, int skipBytes,
    int* firstbyteptr, int* reqbyteptr, CompileData& cd)
{
    const UChar* ptr = *ptrPtr;
    unsigned char* code = *codePtr;
    unsigned char* lastBranch = code;
    unsigned char* start_bracket = code;
    int firstByte = REQ_UNSET;
    int reqByte = REQ_UNSET;
    
    
    
    putLinkValueAllowZero(code + 1, 0);
    code += 1 + LINK_SIZE + skipBytes;
    
    
    
    while (true) {
        
        
        int branchFirstByte;
        int branchReqByte;
        if (!compileBranch(options, brackets, &code, &ptr, patternEnd, errorCodePtr,
                            &branchFirstByte, &branchReqByte, cd)) {
            *ptrPtr = ptr;
            return false;
        }
        
        

        
        if (*lastBranch != OP_ALT) {
            firstByte = branchFirstByte;
            reqByte = branchReqByte;
        }
        
        



        
        else {
            


            
            if (firstByte >= 0 && firstByte != branchFirstByte) {
                if (reqByte < 0)
                    reqByte = firstByte;
                firstByte = REQ_NONE;
            }
            
            

            
            if (firstByte < 0 && branchFirstByte >= 0 && branchReqByte < 0)
                branchReqByte = branchFirstByte;
            
            
            
            if ((reqByte & ~REQ_VARY) != (branchReqByte & ~REQ_VARY))
                reqByte = REQ_NONE;
            else
                reqByte |= branchReqByte;   
        }
        
        





        
        if (ptr >= patternEnd || *ptr != '|') {
            int length = code - lastBranch;
            do {
                int prevLength = getLinkValueAllowZero(lastBranch + 1);
                putLinkValue(lastBranch + 1, length);
                length = prevLength;
                lastBranch -= length;
            } while (length > 0);
            
            
            
            *code = OP_KET;
            putLinkValue(code + 1, code - start_bracket);
            code += 1 + LINK_SIZE;
            
            
            
            *codePtr = code;
            *ptrPtr = ptr;
            *firstbyteptr = firstByte;
            *reqbyteptr = reqByte;
            return true;
        }
        
        



        
        *code = OP_ALT;
        putLinkValue(code + 1, code - lastBranch);
        lastBranch = code;
        code += 1 + LINK_SIZE;
        ptr++;
    }
    JS_NOT_REACHED("No fallthru.");
}


















static bool branchIsAnchored(const unsigned char* code)
{
    const unsigned char* scode = firstSignificantOpcode(code);
    int op = *scode;

    
    if (op >= OP_BRA || op == OP_ASSERT)
        return bracketIsAnchored(scode);

        
    return op == OP_CIRC;
}

static bool bracketIsAnchored(const unsigned char* code)
{
    do {
        if (!branchIsAnchored(code + 1 + LINK_SIZE))
            return false;
        code += getLinkValue(code + 1);
    } while (*code == OP_ALT);   
    return true;
}























static bool branchNeedsLineStart(const unsigned char* code, unsigned captureMap, unsigned backrefMap)
{
    const unsigned char* scode = firstSignificantOpcode(code);
    int op = *scode;
    
    
    if (op > OP_BRA) {
        int captureNum = op - OP_BRA;
        if (captureNum > EXTRACT_BASIC_MAX)
            captureNum = get2ByteValue(scode + 2 + LINK_SIZE);
        int bracketMask = (captureNum < 32) ? (1 << captureNum) : 1;
        return bracketNeedsLineStart(scode, captureMap | bracketMask, backrefMap);
    }
    
    
    if (op == OP_BRA || op == OP_ASSERT)
        return bracketNeedsLineStart(scode, captureMap, backrefMap);
    
    

    
    if (op == OP_TYPESTAR || op == OP_TYPEMINSTAR)
        return scode[1] == OP_NOT_NEWLINE && !(captureMap & backrefMap);

    
    return op == OP_CIRC || op == OP_BOL;
}

static bool bracketNeedsLineStart(const unsigned char* code, unsigned captureMap, unsigned backrefMap)
{
    do {
        if (!branchNeedsLineStart(code + 1 + LINK_SIZE, captureMap, backrefMap))
            return false;
        code += getLinkValue(code + 1);
    } while (*code == OP_ALT);  
    return true;
}





















static int branchFindFirstAssertedCharacter(const unsigned char* code, bool inassert)
{
    const unsigned char* scode = firstSignificantOpcodeSkippingAssertions(code);
    int op = *scode;
    
    if (op >= OP_BRA)
        op = OP_BRA;
    
    switch (op) {
        default:
            return -1;
            
        case OP_BRA:
        case OP_ASSERT:
            return bracketFindFirstAssertedCharacter(scode, op == OP_ASSERT);

        case OP_EXACT:
            scode += 2;
            

        case OP_CHAR:
        case OP_CHAR_IGNORING_CASE:
        case OP_ASCII_CHAR:
        case OP_ASCII_LETTER_IGNORING_CASE:
        case OP_PLUS:
        case OP_MINPLUS:
            if (!inassert)
                return -1;
            return scode[1];
    }
}

static int bracketFindFirstAssertedCharacter(const unsigned char* code, bool inassert)
{
    int c = -1;
    do {
        int d = branchFindFirstAssertedCharacter(code + 1 + LINK_SIZE, inassert);
        if (d < 0)
            return -1;
        if (c < 0)
            c = d;
        else if (c != d)
            return -1;
        code += getLinkValue(code + 1);
    } while (*code == OP_ALT);
    return c;
}

static inline int multiplyWithOverflowCheck(int a, int b)
{
    if (!a || !b)
        return 0;
    if (a > MAX_PATTERN_SIZE / b)
        return -1;
    return a * b;
}

static int calculateCompiledPatternLength(const UChar* pattern, int patternLength, JSRegExpIgnoreCaseOption ignoreCase,
    CompileData& cd, ErrorCode& errorcode)
{
    



    if (patternLength > MAX_PATTERN_SIZE) {
        errorcode = ERR16;
        return -1;
    }

    int length = BRA_LEN;      
    int branch_extra = 0;
    int lastitemlength = 0;
    unsigned brastackptr = 0;
    int brastack[BRASTACK_SIZE];
    unsigned char bralenstack[BRASTACK_SIZE];
    int bracount = 0;
    
    const UChar* ptr = (const UChar*)(pattern - 1);
    const UChar* patternEnd = (const UChar*)(pattern + patternLength);
    
    while (++ptr < patternEnd) {
        int minRepeats = 0, maxRepeats = 0;
        int c = *ptr;

        switch (c) {
            


            case '\\':
                c = checkEscape(&ptr, patternEnd, &errorcode, cd.numCapturingBrackets, false);
                if (errorcode != 0)
                    return -1;
                
                lastitemlength = 1;     
                
                if (c >= 0) {            
                    length += 2;          
                    
                    if (c > 127) {
                        int i;
                        for (i = 0; i < jsc_pcre_utf8_table1_size; i++)
                            if (c <= jsc_pcre_utf8_table1[i]) break;
                        length += i;
                        lastitemlength += i;
                    }
                    
                    continue;
                }
                
                
                
                length++;
                
                


                
                if (c <= -ESC_REF) {
                    int refnum = -c - ESC_REF;
                    cd.backrefMap |= (refnum < 32) ? (1 << refnum) : 1;
                    if (refnum > cd.topBackref)
                        cd.topBackref = refnum;
                    length += 2;   
                    if (safelyCheckNextChar(ptr, patternEnd, '{') && isCountedRepeat(ptr + 2, patternEnd)) {
                        ptr = readRepeatCounts(ptr + 2, &minRepeats, &maxRepeats, &errorcode);
                        if (errorcode)
                            return -1;
                        if ((minRepeats == 0 && (maxRepeats == 1 || maxRepeats == -1)) ||
                            (minRepeats == 1 && maxRepeats == -1))
                            length++;
                        else
                            length += 5;
                        if (safelyCheckNextChar(ptr, patternEnd, '?'))
                            ptr++;
                    }
                }
                continue;
                
            case '^':     
            case '.':
            case '$':
                length++;
                lastitemlength = 1;
                continue;
                
            case '*':            
            case '+':            
            case '?':
                length++;
                goto POSSESSIVE;
                
            


            case '{':
                if (!isCountedRepeat(ptr + 1, patternEnd))
                    goto NORMAL_CHAR;
                ptr = readRepeatCounts(ptr + 1, &minRepeats, &maxRepeats, &errorcode);
                if (errorcode != 0)
                    return -1;
                
                
                
                if ((minRepeats == 0 && (maxRepeats == 1 || maxRepeats == -1)) ||
                    (minRepeats == 1 && maxRepeats == -1))
                    length++;
                
                
                
                else {
                    if (minRepeats != 1) {
                        length -= lastitemlength;   
                        if (minRepeats > 0)
                            length += 5 + lastitemlength;
                    }
                    length += lastitemlength + ((maxRepeats > 0) ? 5 : 1);
                }
                
                if (safelyCheckNextChar(ptr, patternEnd, '?'))
                    ptr++;      

            POSSESSIVE:                     
                if (safelyCheckNextChar(ptr, patternEnd, '+')) {
                    ptr++;
                    length += 2 + 2 * LINK_SIZE;   
                }
                continue;
                
            



                
            case '|':
                if (brastackptr == 0)
                    cd.needOuterBracket = true;
                length += 1 + LINK_SIZE + branch_extra;
                continue;
                
            






                
            case '[': {
                int class_optcount;
                if (*(++ptr) == '^') {
                    class_optcount = 10;  
                    ptr++;
                }
                else
                    class_optcount = 0;
                
                bool class_utf8 = false;
                
                for (; ptr < patternEnd && *ptr != ']'; ++ptr) {
                    
                    
                    if (*ptr == '\\') {
                        c = checkEscape(&ptr, patternEnd, &errorcode, cd.numCapturingBrackets, true);
                        if (errorcode != 0)
                            return -1;
                        
                        
                        
                        if (c >= 0)
                            goto NON_SPECIAL_CHARACTER;
                        
                        

                        
                        else
                            class_optcount = 10;         
                    }
                    
                    




                    
                    else {
                        c = *ptr;
                        
                        
                        
                    NON_SPECIAL_CHARACTER:
                        class_optcount++;
                        
                        int d = -1;
                        if (safelyCheckNextChar(ptr, patternEnd, '-')) {
                            const UChar* hyptr = ptr++;
                            if (safelyCheckNextChar(ptr, patternEnd, '\\')) {
                                ptr++;
                                d = checkEscape(&ptr, patternEnd, &errorcode, cd.numCapturingBrackets, true);
                                if (errorcode != 0)
                                    return -1;
                            }
                            else if ((ptr + 1 < patternEnd) && ptr[1] != ']')
                                d = *++ptr;
                            if (d < 0)
                                ptr = hyptr;      
                        }
                        
                        

                        
                        if (d >= 0) {
                            class_optcount = 10;     
                            if (d < c) {
                                errorcode = ERR8;
                                return -1;
                            }
                            
                            if ((d > 255 || (ignoreCase && d > 127))) {
                                unsigned char buffer[6];
                                if (!class_utf8)         
                                {
                                    class_utf8 = true;
                                    length += LINK_SIZE + 2;
                                }
                                
                                




                                
                                if (ignoreCase) {
                                    int occ, ocd;
                                    int cc = c;
                                    int origd = d;
                                    while (getOthercaseRange(&cc, origd, &occ, &ocd)) {
                                        if (occ >= c && ocd <= d)
                                            continue;   
                                        
                                        if (occ < c  && ocd >= c - 1)  
                                        {                            
                                            c = occ;                     
                                            continue;                    
                                        }                            
                                        if (ocd > d && occ <= d + 1)   
                                        {                            
                                            d = ocd;
                                            continue;
                                        }
                                        
                                        
                                        
                                        length += 1 + encodeUTF8(occ, buffer) +
                                        ((occ == ocd) ? 0 : encodeUTF8(ocd, buffer));
                                    }
                                }
                                
                                
                                
                                length += 1 + encodeUTF8(c, buffer) + encodeUTF8(d, buffer);
                            }
                            
                        }
                        
                        



                        
                        else {
                            if ((c > 255 || (ignoreCase && c > 127))) {
                                unsigned char buffer[6];
                                class_optcount = 10;     
                                if (!class_utf8)         
                                {
                                    class_utf8 = true;
                                    length += LINK_SIZE + 2;
                                }
                                length += (ignoreCase ? 2 : 1) * (1 + encodeUTF8(c, buffer));
                            }
                        }
                    }
                }
                
                if (ptr >= patternEnd) {   
                    errorcode = ERR6;
                    return -1;
                }
                
                




                if (class_optcount == 1)
                    goto NORMAL_CHAR;

                
                {
                    length += 33;
                    
                    

                    
                    if (safelyCheckNextChar(ptr, patternEnd, '{') && isCountedRepeat(ptr + 2, patternEnd)) {
                        ptr = readRepeatCounts(ptr + 2, &minRepeats, &maxRepeats, &errorcode);
                        if (errorcode != 0)
                            return -1;
                        if ((minRepeats == 0 && (maxRepeats == 1 || maxRepeats == -1)) ||
                            (minRepeats == 1 && maxRepeats == -1))
                            length++;
                        else
                            length += 5;
                        if (safelyCheckNextChar(ptr, patternEnd, '+')) {
                            ptr++;
                            length += 2 + 2 * LINK_SIZE;
                        } else if (safelyCheckNextChar(ptr, patternEnd, '?'))
                            ptr++;
                    }
                }
                continue;
            }

            
                
            case '(': {
                int branch_newextra = 0;
                int bracket_length = BRA_LEN;
                bool capturing = false;
                
                
                
                if (safelyCheckNextChar(ptr, patternEnd, '?')) {
                    switch (c = (ptr + 2 < patternEnd ? ptr[2] : 0)) {
                        



                            
                        case ':':
                        case '=':
                        case '!':
                            ptr += 2;
                            break;
                            
                        



                            
                        default:
                            errorcode = ERR12;
                            return -1;
                    }
                } else
                    capturing = true;
                
                


                
                if (capturing) {
                    bracount++;
                    if (bracount > EXTRACT_BASIC_MAX)
                        bracket_length += 3;
                }
                
                



                
                if (brastackptr >= sizeof(brastack)/sizeof(int)) {
                    errorcode = ERR17;
                    return -1;
                }
                
                bralenstack[brastackptr] = branch_extra;
                branch_extra = branch_newextra;
                
                brastack[brastackptr++] = length;
                length += bracket_length;
                continue;
            }

            





            case ')': {
                int duplength;
                length += KET_LEN;
                if (brastackptr > 0) {
                    duplength = length - brastack[--brastackptr];
                    branch_extra = bralenstack[brastackptr];
                }
                else
                    duplength = 0;
                
                

                
                if ((ptr + 1 < patternEnd) && (c = ptr[1]) == '{' && isCountedRepeat(ptr + 2, patternEnd)) {
                    ptr = readRepeatCounts(ptr + 2, &minRepeats, &maxRepeats, &errorcode);
                    if (errorcode)
                        return -1;
                } else if (c == '*') {
                    minRepeats = 0;
                    maxRepeats = -1;
                    ptr++;
                } else if (c == '+') {
                    minRepeats = 1;
                    maxRepeats = -1;
                    ptr++;
                } else if (c == '?') {
                    minRepeats = 0;
                    maxRepeats = 1;
                    ptr++;
                } else {
                    minRepeats = 1;
                    maxRepeats = 1;
                }
                
                



                
                int repeatsLength;
                if (minRepeats == 0) {
                    length++;
                    if (maxRepeats > 0) {
                        repeatsLength = multiplyWithOverflowCheck(maxRepeats - 1, duplength + BRA_LEN + KET_LEN + OPCODE_LEN);
                        if (repeatsLength < 0) {
                            errorcode = ERR16;
                            return -1;
                        }
                        length += repeatsLength;
                        if (length > MAX_PATTERN_SIZE) {
                            errorcode = ERR16;
                            return -1;
                        }
                    }
                }
                
                




                
                else {
                    repeatsLength = multiplyWithOverflowCheck(minRepeats - 1, duplength);
                    if (repeatsLength < 0) {
                        errorcode = ERR16;
                        return -1;
                    }
                    length += repeatsLength;
                    if (maxRepeats > minRepeats) { 
                        repeatsLength = multiplyWithOverflowCheck(maxRepeats - minRepeats, duplength + BRAZERO_LEN + BRA_LEN + KET_LEN);
                        if (repeatsLength < 0) {
                            errorcode = ERR16;
                            return -1;
                        }
                        length += repeatsLength - (2 + 2 * LINK_SIZE);
                    }
                    if (length > MAX_PATTERN_SIZE) {
                        errorcode = ERR16;
                        return -1;
                    }
                }
                
                
                
                if (safelyCheckNextChar(ptr, patternEnd, '+')) {
                    ptr++;
                    length += 2 + 2 * LINK_SIZE;
                }
                continue;
            }

            


                
            default:
            NORMAL_CHAR:
                length += 2;          
                lastitemlength = 1;   

                if (c > 127) {
                    int i;
                    for (i = 0; i < jsc_pcre_utf8_table1_size; i++)
                        if (c <= jsc_pcre_utf8_table1[i])
                            break;
                    length += i;
                    lastitemlength += i;
                }
                
                continue;
        }
    }
    
    length += KET_LEN + OPCODE_LEN;    

    cd.numCapturingBrackets = bracount;
    return length;
}























static inline JSRegExp* returnError(ErrorCode errorcode, int *error)
{
    *error = static_cast<int>(errorcode);
    return 0;
}

JSRegExp* jsRegExpCompile(const UChar* pattern, int patternLength,
                JSRegExpIgnoreCaseOption ignoreCase, JSRegExpMultilineOption multiline,
                unsigned* numSubpatterns, int *error)
{
    

    if (!error)
        return 0;
    *error = 0;
    
    CompileData cd;
    
    ErrorCode errorcode = ERR0;
    
    calculateCompiledPatternLength(pattern, patternLength, ignoreCase, cd, errorcode);
    
    int length = calculateCompiledPatternLength(pattern, patternLength, ignoreCase, cd, errorcode);
    if (errorcode)
        return returnError(errorcode, error);
    
    if (length > MAX_PATTERN_SIZE)
        return returnError(ERR16, error);
    
    size_t size = length + sizeof(JSRegExp);
    JSRegExp* re = reinterpret_cast<JSRegExp*>(js::OffTheBooks::array_new<char>(size));
    if (!re)
        return returnError(ERR13, error);
    
    re->options = (ignoreCase ? IgnoreCaseOption : 0) | (multiline ? MatchAcrossMultipleLinesOption : 0);
    
    

    
    const unsigned char* codeStart = (const unsigned char*)(re + 1);
    
    


    
    const UChar* ptr = (const UChar*)pattern;
    const UChar* patternEnd = pattern + patternLength;
    unsigned char* code = const_cast<unsigned char*>(codeStart);
    int firstByte, reqByte;
    int bracketCount = 0;
    if (!cd.needOuterBracket)
        compileBranch(re->options, &bracketCount, &code, &ptr, patternEnd, &errorcode, &firstByte, &reqByte, cd);
    else {
        *code = OP_BRA;
        unsigned char * const codeBefore = code;
        compileBracket(re->options, &bracketCount, &code, &ptr, patternEnd, &errorcode, 2, &firstByte, &reqByte, cd);
        JS_ASSERT((bracketCount & 0xff) == bracketCount);
        put2ByteValue(codeBefore + 1 + LINK_SIZE, 0 << 8 | (bracketCount & 0xff));
    }
    re->topBracket = bracketCount;
    re->topBackref = cd.topBackref;
    
    
    
    if (errorcode == 0 && ptr < patternEnd)
        errorcode = ERR10;
    
    

    
    *code++ = OP_END;

    JS_ASSERT(code - codeStart <= length);
    if (code - codeStart > length)
        errorcode = ERR7;
    
    

    
    if (re->topBackref > re->topBracket)
        errorcode = ERR15;
    
    
    
    if (errorcode != ERR0) {
        js::Foreground::array_delete(reinterpret_cast<char*>(re));
        return returnError(errorcode, error);
    }
    
    








    
    if (cd.needOuterBracket ? bracketIsAnchored(codeStart) : branchIsAnchored(codeStart))
        re->options |= IsAnchoredOption;
    else {
        if (firstByte < 0) {
            firstByte = (cd.needOuterBracket
                    ? bracketFindFirstAssertedCharacter(codeStart, false)
                    : branchFindFirstAssertedCharacter(codeStart, false))
                | ((re->options & IgnoreCaseOption) ? REQ_IGNORE_CASE : 0);
        }
        if (firstByte >= 0) {
            int ch = firstByte & 255;
            if (ch < 127) {
                re->firstByte = ((firstByte & REQ_IGNORE_CASE) && flipCase(ch) == ch) ? ch : firstByte;
                re->options |= UseFirstByteOptimizationOption;
            }
        } else {
            if (cd.needOuterBracket ? bracketNeedsLineStart(codeStart, 0, cd.backrefMap) : branchNeedsLineStart(codeStart, 0, cd.backrefMap))
                re->options |= UseMultiLineFirstByteOptimizationOption;
        }
    }
    
    


    
    if (reqByte >= 0 && (!(re->options & IsAnchoredOption) || (reqByte & REQ_VARY))) {
        int ch = reqByte & 255;
        if (ch < 127) {
            re->reqByte = ((reqByte & REQ_IGNORE_CASE) && flipCase(ch) == ch) ? (reqByte & ~REQ_IGNORE_CASE) : reqByte;
            re->options |= UseRequiredByteOptimizationOption;
        }
    }
    
    if (numSubpatterns)
        *numSubpatterns = re->topBracket;

    return re;
}

void jsRegExpFree(JSRegExp* re)
{
    js::Foreground::array_delete(reinterpret_cast<char*>(re));
}
