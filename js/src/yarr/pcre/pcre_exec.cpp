











































#include "pcre_internal.h"

#include <limits.h>
#include "yarr/jswtfbridge.h"
#include "yarr/wtf/ASCIICType.h"
#include "jsarena.h"
#include "jscntxt.h"

using namespace WTF;

#if !WTF_COMPILER_MSVC && !WTF_COMPILER_SUNPRO
#define USE_COMPUTED_GOTO_FOR_MATCH_RECURSION
#endif











#undef min
#undef max

#ifndef USE_COMPUTED_GOTO_FOR_MATCH_RECURSION
typedef int ReturnLocation;
#else
typedef void* ReturnLocation;
#endif





struct BracketChainNode {
    BracketChainNode* previousBracket;
    const UChar* bracketStart;
    

    bool minSatisfied;
};

struct MatchFrame {
    ReturnLocation returnLocation;
    struct MatchFrame* previousFrame;
    int *savedOffsets;
    

    size_t savedOffsetsSize;
    JSArenaPool *regExpPool;

    MatchFrame() : savedOffsetsSize(0), regExpPool(0) {}
    void init(JSArenaPool *regExpPool) { this->regExpPool = regExpPool; }
    
    
    struct {
        const UChar* subjectPtr;
        const unsigned char* instructionPtr;
        int offsetTop;
        BracketChainNode* bracketChain;
    } args;
    
    
    


    struct {
        const unsigned char* data;
        const unsigned char* startOfRepeatingBracket;
        const UChar* subjectPtrAtStartOfInstruction; 
        const unsigned char* instructionPtrAtStartOfOnce;
        
        int repeatOthercase;
        int savedSubjectOffset;
        
        int ctype;
        int fc;
        int fi;
        int length;
        int max;
        int number;
        int offset;
        int skipBytes;
        int minBracket;
        int limitBracket;
        int bracketsBefore;
        bool minSatisfied;
        
        BracketChainNode bracketChainNode;
    } locals;

    void saveOffsets(int minBracket, int limitBracket, int *offsets, int offsetEnd) {
        JS_ASSERT(regExpPool);
        JS_ASSERT(minBracket >= 0);
        JS_ASSERT(limitBracket >= minBracket);
        JS_ASSERT(offsetEnd >= 0);
        if (minBracket == limitBracket)
            return;
        const size_t newSavedOffsetCount = 3 * (limitBracket - minBracket);
        
        {
            size_t targetSize = sizeof(*savedOffsets) * newSavedOffsetCount;
            if (savedOffsetsSize < targetSize) {
                JS_ARENA_ALLOCATE_CAST(savedOffsets, int *, regExpPool, targetSize);
                JS_ASSERT(savedOffsets); 
                savedOffsetsSize = targetSize;
            }
        }
        for (unsigned i = 0; i < unsigned(limitBracket - minBracket); ++i) {
            int bracketIter = minBracket + i;
            JS_ASSERT(2 * bracketIter + 1 <= offsetEnd);
            int start = offsets[2 * bracketIter];
            int end = offsets[2 * bracketIter + 1];
            JS_ASSERT(bracketIter <= offsetEnd);
            int offset = offsets[offsetEnd - bracketIter];
            DPRINTF(("saving bracket %d; start: %d; end: %d; offset: %d\n", bracketIter, start, end, offset));
            JS_ASSERT(start <= end);
            JS_ASSERT(i * 3 + 2 < newSavedOffsetCount);
            savedOffsets[i * 3 + 0] = start;
            savedOffsets[i * 3 + 1] = end;
            savedOffsets[i * 3 + 2] = offset;
        }
    }

    void clobberOffsets(int minBracket, int limitBracket, int *offsets, int offsetEnd) {
        for (int i = 0; i < limitBracket - minBracket; ++i) {
            int bracketIter = minBracket + i;
            JS_ASSERT(2 * bracketIter + 1 < offsetEnd);
            offsets[2 * bracketIter + 0] = -1;
            offsets[2 * bracketIter + 1] = -1;
        }
    }

    void restoreOffsets(int minBracket, int limitBracket, int *offsets, int offsetEnd) {
        JS_ASSERT(regExpPool);
        JS_ASSERT_IF(limitBracket > minBracket, savedOffsets);
        for (int i = 0; i < limitBracket - minBracket; ++i) {
            int bracketIter = minBracket + i;
            int start = savedOffsets[i * 3 + 0];
            int end = savedOffsets[i * 3 + 1];
            int offset = savedOffsets[i * 3 + 2];
            DPRINTF(("restoring bracket %d; start: %d; end: %d; offset: %d\n", bracketIter, start, end, offset));
            JS_ASSERT(start <= end);
            offsets[2 * bracketIter + 0] = start;
            offsets[2 * bracketIter + 1] = end;
            offsets[offsetEnd - bracketIter] = offset;
        }
    }

    
    void extractBrackets(const unsigned char *instructionPtr) {
        uint16 bracketMess = get2ByteValue(instructionPtr + 1 + LINK_SIZE);
        locals.minBracket = (bracketMess >> 8) & 0xff;
        locals.limitBracket = (bracketMess & 0xff);
        JS_ASSERT(locals.minBracket <= locals.limitBracket);
    }

    



    void startNewGroup(bool minSatisfied) {
        locals.bracketChainNode.previousBracket = args.bracketChain;
        locals.bracketChainNode.bracketStart = args.subjectPtr;
        locals.bracketChainNode.minSatisfied = minSatisfied;
        args.bracketChain = &locals.bracketChainNode;
    }
};




struct MatchData {
    int             *offsetVector;  
    int             offsetEnd;      
    int             offsetMax;      
    bool            offsetOverflow; 
    const UChar     *startSubject;  
    const UChar     *endSubject;    
    const UChar     *endMatchPtr;   
    int             endOffsetTop;   
    bool            multiline;
    bool            ignoreCase;

    void setOffsetPair(size_t pairNum, int start, int end) {
        JS_ASSERT(int(2 * pairNum + 1) < offsetEnd && int(pairNum) < offsetEnd);
        JS_ASSERT(start <= end);
        JS_ASSERT_IF(start < 0, start == end && start == -1);
        DPRINTF(("setting offset pair at %u (%d, %d)\n", pairNum, start, end));
        offsetVector[2 * pairNum + 0] = start;
        offsetVector[2 * pairNum + 1] = end;
    }
};




#define REQ_BYTE_MAX 1000




static const unsigned matchLimit = 1000000;

















static bool matchRef(int offset, const UChar* subjectPtr, int length, const MatchData& md)
{
    const UChar* p = md.startSubject + md.offsetVector[offset];
    
    
    
    if (length > md.endSubject - subjectPtr)
        return false;
    
    
    
    if (md.ignoreCase) {
        while (length-- > 0) {
            UChar c = *p++;
            int othercase = jsc_pcre_ucp_othercase(c);
            UChar d = *subjectPtr++;
            if (c != d && othercase != d)
                return false;
        }
    }
    else {
        while (length-- > 0)
            if (*p++ != *subjectPtr++)
                return false;
    }
    
    return true;
}

#ifndef USE_COMPUTED_GOTO_FOR_MATCH_RECURSION



#define RMATCH_WHERE(num) num
#define RRETURN_LABEL RRETURN_SWITCH

#else








#define RMATCH_WHERE(num) JS_EXTENSION(&&RRETURN_##num)
#define RRETURN_LABEL *stack.currentFrame->returnLocation

#endif

#define RECURSIVE_MATCH_COMMON(num) \
    goto RECURSE;\
    RRETURN_##num: \
    stack.popCurrentFrame();

#define RECURSIVE_MATCH(num, ra, rb) \
    do { \
        stack.pushNewFrame((ra), (rb), RMATCH_WHERE(num)); \
        RECURSIVE_MATCH_COMMON(num) \
    } while (0)

#define RECURSIVE_MATCH_NEW_GROUP(num, ra, rb, gm) \
    do { \
        stack.pushNewFrame((ra), (rb), RMATCH_WHERE(num)); \
        stack.currentFrame->startNewGroup(gm); \
        RECURSIVE_MATCH_COMMON(num) \
    } while (0)

#define RRETURN do { JS_EXTENSION_(goto RRETURN_LABEL); } while (0)

#define RRETURN_NO_MATCH do { isMatch = false; RRETURN; } while (0)
























static const unsigned numFramesOnStack = 16;

struct MatchStack {
    JSArenaPool *regExpPool;
    void *regExpPoolMark;

    MatchStack(JSArenaPool *regExpPool)
        : regExpPool(regExpPool)
        , regExpPoolMark(JS_ARENA_MARK(regExpPool))
        , framesEnd(frames + numFramesOnStack)
        , currentFrame(frames)
        , size(1) 
    {
        JS_ASSERT((sizeof(frames) / sizeof(frames[0])) == numFramesOnStack);
        JS_ASSERT(regExpPool);
        for (size_t i = 0; i < numFramesOnStack; ++i)
            frames[i].init(regExpPool);
    }

    ~MatchStack() { JS_ARENA_RELEASE(regExpPool, regExpPoolMark); }
    
    MatchFrame frames[numFramesOnStack];
    MatchFrame* framesEnd;
    MatchFrame* currentFrame;
    unsigned size;
    
    bool canUseStackBufferForNextFrame() {
        return size < numFramesOnStack;
    }
    
    MatchFrame* allocateNextFrame() {
        if (canUseStackBufferForNextFrame())
            return currentFrame + 1;
        
        MatchFrame *frame = js::OffTheBooks::new_<MatchFrame>();
        frame->init(regExpPool);
        return frame;
    }
    
    void pushNewFrame(const unsigned char* instructionPtr, BracketChainNode* bracketChain, ReturnLocation returnLocation) {
        MatchFrame* newframe = allocateNextFrame();
        newframe->previousFrame = currentFrame;

        newframe->args.subjectPtr = currentFrame->args.subjectPtr;
        newframe->args.offsetTop = currentFrame->args.offsetTop;
        newframe->args.instructionPtr = instructionPtr;
        newframe->args.bracketChain = bracketChain;
        newframe->returnLocation = returnLocation;
        size++;

        currentFrame = newframe;
    }
    
    void popCurrentFrame() {
        MatchFrame* oldFrame = currentFrame;
        currentFrame = currentFrame->previousFrame;
        if (size > numFramesOnStack)
            js::Foreground::delete_(oldFrame);
        size--;
    }

    void popAllFrames() {
        while (size)
            popCurrentFrame();
    }
};

static int matchError(int errorCode, MatchStack& stack)
{
    stack.popAllFrames();
    return errorCode;
}




static inline void getUTF8CharAndIncrementLength(int& c, const unsigned char* subjectPtr, int& len)
{
    c = *subjectPtr;
    if ((c & 0xc0) == 0xc0) {
        int gcaa = jsc_pcre_utf8_table4[c & 0x3f];  
        int gcss = 6 * gcaa;
        c = (c & jsc_pcre_utf8_table3[gcaa]) << gcss;
        for (int gcii = 1; gcii <= gcaa; gcii++) {
            gcss -= 6;
            c |= (subjectPtr[gcii] & 0x3f) << gcss;
        }
        len += gcaa;
    }
}

static inline void repeatInformationFromInstructionOffset(short instructionOffset, bool& minimize, int& minimumRepeats, int& maximumRepeats)
{
    
    static const char minimumRepeatsFromInstructionOffset[] = { 0, 0, 1, 1, 0, 0 };
    static const int maximumRepeatsFromInstructionOffset[] = { INT_MAX, INT_MAX, INT_MAX, INT_MAX, 1, 1 };

    JS_ASSERT(instructionOffset >= 0);
    JS_ASSERT(instructionOffset <= (OP_CRMINQUERY - OP_CRSTAR));

    minimize = (instructionOffset & 1); 
    minimumRepeats = minimumRepeatsFromInstructionOffset[instructionOffset];
    maximumRepeats = maximumRepeatsFromInstructionOffset[instructionOffset];
}




class LinearFlag {
public:
    LinearFlag() : flag(false) {}
    
    bool readAndClear() {
        bool rv = flag;
        flag = false;
        return rv;
    }

    void set() {
        flag = true;
    }

private:
    bool flag;
};

static int
match(JSArenaPool *regExpPool, const UChar* subjectPtr, const unsigned char* instructionPtr, int offsetTop, MatchData& md)
{
    bool isMatch = false;
    int min;
    bool minimize = false; 
    unsigned remainingMatchCount = matchLimit;
    int othercase; 
    bool minSatisfied;
    
    MatchStack stack(regExpPool);
    LinearFlag minSatNextBracket;

    
#ifdef USE_COMPUTED_GOTO_FOR_MATCH_OPCODE_LOOP
#define EMIT_JUMP_TABLE_ENTRY(opcode) JS_EXTENSION(&&LABEL_OP_##opcode)
    static void* opcodeJumpTable[256] = { FOR_EACH_OPCODE(EMIT_JUMP_TABLE_ENTRY) };
#undef EMIT_JUMP_TABLE_ENTRY
#endif
    
    
#ifdef USE_COMPUTED_GOTO_FOR_MATCH_OPCODE_LOOP
    for (int i = 255; !opcodeJumpTable[i]; i--)
        opcodeJumpTable[i] = &&CAPTURING_BRACKET;
#endif
    
#ifdef USE_COMPUTED_GOTO_FOR_MATCH_RECURSION
    
    
    stack.currentFrame->returnLocation = JS_EXTENSION(&&RETURN);
#else
    stack.currentFrame->returnLocation = 0;
#endif
    stack.currentFrame->args.subjectPtr = subjectPtr;
    stack.currentFrame->args.instructionPtr = instructionPtr;
    stack.currentFrame->args.offsetTop = offsetTop;
    stack.currentFrame->args.bracketChain = 0;
    stack.currentFrame->startNewGroup(false);
    
    
    
RECURSE:
    if (!--remainingMatchCount)
        return matchError(JSRegExpErrorHitLimit, stack);

    
    
#ifndef USE_COMPUTED_GOTO_FOR_MATCH_OPCODE_LOOP
    while (true)
#endif
    {
        
#ifdef USE_COMPUTED_GOTO_FOR_MATCH_OPCODE_LOOP
#define BEGIN_OPCODE(opcode) LABEL_OP_##opcode
#define NEXT_OPCODE goto *opcodeJumpTable[*stack.currentFrame->args.instructionPtr]
#else
#define BEGIN_OPCODE(opcode) case OP_##opcode
#define NEXT_OPCODE continue
#endif
#define LOCALS(__ident) (stack.currentFrame->locals.__ident)
        
#ifdef USE_COMPUTED_GOTO_FOR_MATCH_OPCODE_LOOP
        NEXT_OPCODE;
#else
        switch (*stack.currentFrame->args.instructionPtr)
#endif
        {
            
                
            BEGIN_OPCODE(BRA):
            NON_CAPTURING_BRACKET:
                DPRINTF(("start non-capturing bracket\n"));
                stack.currentFrame->extractBrackets(stack.currentFrame->args.instructionPtr);
                

                stack.currentFrame->locals.skipBytes = 3;
                
                stack.currentFrame->locals.minSatisfied = minSatNextBracket.readAndClear();
                do {
                    

                    minSatisfied = stack.currentFrame->locals.minSatisfied;
                    RECURSIVE_MATCH_NEW_GROUP(2, stack.currentFrame->args.instructionPtr + stack.currentFrame->locals.skipBytes + LINK_SIZE, stack.currentFrame->args.bracketChain, minSatisfied);
                    if (isMatch) {
                        DPRINTF(("non-capturing bracket succeeded\n"));
                        RRETURN;
                    }
                    stack.currentFrame->locals.skipBytes = 1;
                    stack.currentFrame->args.instructionPtr += getLinkValue(stack.currentFrame->args.instructionPtr + 1);
                } while (*stack.currentFrame->args.instructionPtr == OP_ALT);
                DPRINTF(("non-capturing bracket failed\n"));
                for (size_t i = LOCALS(minBracket); i < size_t(LOCALS(limitBracket)); ++i)
                    md.setOffsetPair(i, -1, -1);
                RRETURN;
                
            
                
            BEGIN_OPCODE(BRANUMBER):
                stack.currentFrame->args.instructionPtr += 3;
                NEXT_OPCODE;
                
            
                
            BEGIN_OPCODE(END):
                md.endMatchPtr = stack.currentFrame->args.subjectPtr;          
                md.endOffsetTop = stack.currentFrame->args.offsetTop;   
                isMatch = true;
                RRETURN;
                
            




                
            BEGIN_OPCODE(ASSERT):
                {
                    uint16 bracketMess = get2ByteValue(stack.currentFrame->args.instructionPtr + 1 + LINK_SIZE);
                    LOCALS(minBracket) = (bracketMess >> 8) & 0xff;
                    LOCALS(limitBracket) = bracketMess & 0xff;
                    JS_ASSERT(LOCALS(minBracket) <= LOCALS(limitBracket));
                }
                stack.currentFrame->locals.skipBytes = 3;
                do {
                    RECURSIVE_MATCH_NEW_GROUP(6, stack.currentFrame->args.instructionPtr + stack.currentFrame->locals.skipBytes + LINK_SIZE, NULL, false);
                    if (isMatch)
                        break;
                    stack.currentFrame->locals.skipBytes = 1;
                    stack.currentFrame->args.instructionPtr += getLinkValue(stack.currentFrame->args.instructionPtr + 1);
                } while (*stack.currentFrame->args.instructionPtr == OP_ALT);
                if (*stack.currentFrame->args.instructionPtr == OP_KET) {
                    for (size_t i = LOCALS(minBracket); i < size_t(LOCALS(limitBracket)); ++i)
                        md.setOffsetPair(i, -1, -1);
                    RRETURN_NO_MATCH;
                }
                
                

                
                advanceToEndOfBracket(stack.currentFrame->args.instructionPtr);
                stack.currentFrame->args.instructionPtr += 1 + LINK_SIZE;
                stack.currentFrame->args.offsetTop = md.endOffsetTop;
                NEXT_OPCODE;
                
            
                
            BEGIN_OPCODE(ASSERT_NOT):
                stack.currentFrame->locals.skipBytes = 3;
                {
                    unsigned bracketMess = get2ByteValue(stack.currentFrame->args.instructionPtr + 1 + LINK_SIZE);
                    LOCALS(minBracket) = (bracketMess >> 8) & 0xff;
                    LOCALS(limitBracket) = bracketMess & 0xff;
                }
                JS_ASSERT(LOCALS(minBracket) <= LOCALS(limitBracket));
                do {
                    RECURSIVE_MATCH_NEW_GROUP(7, stack.currentFrame->args.instructionPtr + stack.currentFrame->locals.skipBytes + LINK_SIZE, NULL, false);
                    if (isMatch)
                        RRETURN_NO_MATCH;
                    stack.currentFrame->locals.skipBytes = 1;
                    stack.currentFrame->args.instructionPtr += getLinkValue(stack.currentFrame->args.instructionPtr + 1);
                } while (*stack.currentFrame->args.instructionPtr == OP_ALT);
                
                stack.currentFrame->args.instructionPtr += stack.currentFrame->locals.skipBytes + LINK_SIZE;
                NEXT_OPCODE;
                
            

                
            BEGIN_OPCODE(ALT):
                advanceToEndOfBracket(stack.currentFrame->args.instructionPtr);
                NEXT_OPCODE;
                
            




                
            BEGIN_OPCODE(BRAZERO): {
                stack.currentFrame->locals.startOfRepeatingBracket = stack.currentFrame->args.instructionPtr + 1;
                stack.currentFrame->extractBrackets(stack.currentFrame->args.instructionPtr + 1);
                stack.currentFrame->saveOffsets(LOCALS(minBracket), LOCALS(limitBracket), md.offsetVector, md.offsetEnd);
                minSatNextBracket.set();
                RECURSIVE_MATCH_NEW_GROUP(14, stack.currentFrame->locals.startOfRepeatingBracket, stack.currentFrame->args.bracketChain, true);
                if (isMatch)
                    RRETURN;
                stack.currentFrame->restoreOffsets(LOCALS(minBracket), LOCALS(limitBracket), md.offsetVector, md.offsetEnd);
                advanceToEndOfBracket(stack.currentFrame->locals.startOfRepeatingBracket);
                stack.currentFrame->args.instructionPtr = stack.currentFrame->locals.startOfRepeatingBracket + 1 + LINK_SIZE;
                NEXT_OPCODE;
            }
                
            BEGIN_OPCODE(BRAMINZERO): {
                stack.currentFrame->locals.startOfRepeatingBracket = stack.currentFrame->args.instructionPtr + 1;
                advanceToEndOfBracket(stack.currentFrame->locals.startOfRepeatingBracket);
                RECURSIVE_MATCH_NEW_GROUP(15, stack.currentFrame->locals.startOfRepeatingBracket + 1 + LINK_SIZE, stack.currentFrame->args.bracketChain, false);
                if (isMatch)
                    RRETURN;
                stack.currentFrame->args.instructionPtr++;
                NEXT_OPCODE;
            }
                
            



                
            BEGIN_OPCODE(KET):
            BEGIN_OPCODE(KETRMIN):
            BEGIN_OPCODE(KETRMAX):
                stack.currentFrame->locals.instructionPtrAtStartOfOnce = stack.currentFrame->args.instructionPtr - getLinkValue(stack.currentFrame->args.instructionPtr + 1);
                stack.currentFrame->locals.subjectPtrAtStartOfInstruction = stack.currentFrame->args.bracketChain->bracketStart;
                stack.currentFrame->locals.minSatisfied = stack.currentFrame->args.bracketChain->minSatisfied;

                

                stack.currentFrame->args.bracketChain = stack.currentFrame->args.bracketChain->previousBracket;

                if (*stack.currentFrame->locals.instructionPtrAtStartOfOnce == OP_ASSERT || *stack.currentFrame->locals.instructionPtrAtStartOfOnce == OP_ASSERT_NOT) {
                    md.endOffsetTop = stack.currentFrame->args.offsetTop;
                    isMatch = true;
                    RRETURN;
                }
                
                


                
                stack.currentFrame->locals.number = *stack.currentFrame->locals.instructionPtrAtStartOfOnce - OP_BRA;
                
                

                
                if (stack.currentFrame->locals.number > EXTRACT_BASIC_MAX)
                    stack.currentFrame->locals.number = get2ByteValue(stack.currentFrame->locals.instructionPtrAtStartOfOnce + 4 + LINK_SIZE);
                stack.currentFrame->locals.offset = 2 * stack.currentFrame->locals.number;
                
                DPRINTF(("end bracket %d\n", stack.currentFrame->locals.number));
                
                



                
                if (stack.currentFrame->locals.number > 0) {
                    if (stack.currentFrame->locals.offset >= md.offsetMax)
                        md.offsetOverflow = true;
                    else {
                        int start = md.offsetVector[md.offsetEnd - stack.currentFrame->locals.number];
                        int end = stack.currentFrame->args.subjectPtr - md.startSubject;
                        if (start == end && stack.currentFrame->locals.minSatisfied) {
                            DPRINTF(("empty string while group already matched; bailing"));
                            RRETURN_NO_MATCH;
                        }
                        DPRINTF(("saving; start: %d; end: %d\n", start, end));
                        JS_ASSERT(start <= end);
                        md.setOffsetPair(stack.currentFrame->locals.number, start, end);
                        if (stack.currentFrame->args.offsetTop <= stack.currentFrame->locals.offset)
                            stack.currentFrame->args.offsetTop = stack.currentFrame->locals.offset + 2;
                    }
                }
                
                




                
                if (*stack.currentFrame->args.instructionPtr == OP_KET || stack.currentFrame->args.subjectPtr == stack.currentFrame->locals.subjectPtrAtStartOfInstruction) {
                    DPRINTF(("non-repeating ket or empty match\n"));
                    if (stack.currentFrame->args.subjectPtr == stack.currentFrame->locals.subjectPtrAtStartOfInstruction && stack.currentFrame->locals.minSatisfied) {
                        DPRINTF(("empty string while group already matched; bailing"));
                        RRETURN_NO_MATCH;
                    }
                    stack.currentFrame->args.instructionPtr += 1 + LINK_SIZE;
                    NEXT_OPCODE;
                }
                
                

                
                stack.currentFrame->extractBrackets(LOCALS(instructionPtrAtStartOfOnce));
                JS_ASSERT_IF(LOCALS(number), LOCALS(minBracket) <= LOCALS(number) && LOCALS(number) < LOCALS(limitBracket));
                if (*stack.currentFrame->args.instructionPtr == OP_KETRMIN) {
                    stack.currentFrame->saveOffsets(LOCALS(minBracket), LOCALS(limitBracket), md.offsetVector, md.offsetEnd);
                    RECURSIVE_MATCH(16, stack.currentFrame->args.instructionPtr + 1 + LINK_SIZE, stack.currentFrame->args.bracketChain);
                    if (isMatch)
                        RRETURN;
                    else
                        stack.currentFrame->restoreOffsets(LOCALS(minBracket), LOCALS(limitBracket), md.offsetVector, md.offsetEnd);
                    DPRINTF(("recursively matching lazy group\n"));
                    minSatNextBracket.set();
                    RECURSIVE_MATCH_NEW_GROUP(17, LOCALS(instructionPtrAtStartOfOnce), stack.currentFrame->args.bracketChain, true);
                } else { 
                    stack.currentFrame->saveOffsets(LOCALS(minBracket), LOCALS(limitBracket), md.offsetVector, md.offsetEnd);
                    stack.currentFrame->clobberOffsets(LOCALS(minBracket), LOCALS(limitBracket), md.offsetVector, md.offsetEnd);
                    DPRINTF(("recursively matching greedy group\n"));
                    minSatNextBracket.set();
                    RECURSIVE_MATCH_NEW_GROUP(18, LOCALS(instructionPtrAtStartOfOnce), stack.currentFrame->args.bracketChain, true);
                    if (isMatch)
                        RRETURN;
                    else
                        stack.currentFrame->restoreOffsets(LOCALS(minBracket), LOCALS(limitBracket), md.offsetVector, md.offsetEnd);
                    RECURSIVE_MATCH(19, stack.currentFrame->args.instructionPtr + 1 + LINK_SIZE, stack.currentFrame->args.bracketChain);
                }
                RRETURN;
                
            

            BEGIN_OPCODE(CIRC):
                if (stack.currentFrame->args.subjectPtr != md.startSubject)
                    RRETURN_NO_MATCH;
                stack.currentFrame->args.instructionPtr++;
                NEXT_OPCODE;

            

            BEGIN_OPCODE(BOL):
                if (stack.currentFrame->args.subjectPtr != md.startSubject && !isNewline(stack.currentFrame->args.subjectPtr[-1]))
                    RRETURN_NO_MATCH;
                stack.currentFrame->args.instructionPtr++;
                NEXT_OPCODE;

            

            BEGIN_OPCODE(DOLL):
                if (stack.currentFrame->args.subjectPtr < md.endSubject)
                    RRETURN_NO_MATCH;
                stack.currentFrame->args.instructionPtr++;
                NEXT_OPCODE;

            

            BEGIN_OPCODE(EOL):
                if (stack.currentFrame->args.subjectPtr < md.endSubject && !isNewline(*stack.currentFrame->args.subjectPtr))
                    RRETURN_NO_MATCH;
                stack.currentFrame->args.instructionPtr++;
                NEXT_OPCODE;
                
            
                
            BEGIN_OPCODE(NOT_WORD_BOUNDARY):
            BEGIN_OPCODE(WORD_BOUNDARY): {
                bool currentCharIsWordChar = false;
                bool previousCharIsWordChar = false;
                
                if (stack.currentFrame->args.subjectPtr > md.startSubject)
                    previousCharIsWordChar = isWordChar(stack.currentFrame->args.subjectPtr[-1]);
                if (stack.currentFrame->args.subjectPtr < md.endSubject)
                    currentCharIsWordChar = isWordChar(*stack.currentFrame->args.subjectPtr);
                
                
                bool wordBoundaryDesired = (*stack.currentFrame->args.instructionPtr++ == OP_WORD_BOUNDARY);
                if (wordBoundaryDesired ? currentCharIsWordChar == previousCharIsWordChar : currentCharIsWordChar != previousCharIsWordChar)
                    RRETURN_NO_MATCH;
                NEXT_OPCODE;
            }
                
            
                
            BEGIN_OPCODE(NOT_NEWLINE):
                if (stack.currentFrame->args.subjectPtr >= md.endSubject)
                    RRETURN_NO_MATCH;
                if (isNewline(*stack.currentFrame->args.subjectPtr++))
                    RRETURN_NO_MATCH;
                stack.currentFrame->args.instructionPtr++;
                NEXT_OPCODE;

            BEGIN_OPCODE(NOT_DIGIT):
                if (stack.currentFrame->args.subjectPtr >= md.endSubject)
                    RRETURN_NO_MATCH;
                if (isASCIIDigit(*stack.currentFrame->args.subjectPtr++))
                    RRETURN_NO_MATCH;
                stack.currentFrame->args.instructionPtr++;
                NEXT_OPCODE;

            BEGIN_OPCODE(DIGIT):
                if (stack.currentFrame->args.subjectPtr >= md.endSubject)
                    RRETURN_NO_MATCH;
                if (!isASCIIDigit(*stack.currentFrame->args.subjectPtr++))
                    RRETURN_NO_MATCH;
                stack.currentFrame->args.instructionPtr++;
                NEXT_OPCODE;

            BEGIN_OPCODE(NOT_WHITESPACE):
                if (stack.currentFrame->args.subjectPtr >= md.endSubject)
                    RRETURN_NO_MATCH;
                if (isSpaceChar(*stack.currentFrame->args.subjectPtr++))
                    RRETURN_NO_MATCH;
                stack.currentFrame->args.instructionPtr++;
                NEXT_OPCODE;

            BEGIN_OPCODE(WHITESPACE):
                if (stack.currentFrame->args.subjectPtr >= md.endSubject)
                    RRETURN_NO_MATCH;
                if (!isSpaceChar(*stack.currentFrame->args.subjectPtr++))
                    RRETURN_NO_MATCH;
                stack.currentFrame->args.instructionPtr++;
                NEXT_OPCODE;
                
            BEGIN_OPCODE(NOT_WORDCHAR):
                if (stack.currentFrame->args.subjectPtr >= md.endSubject)
                    RRETURN_NO_MATCH;
                if (isWordChar(*stack.currentFrame->args.subjectPtr++))
                    RRETURN_NO_MATCH;
                stack.currentFrame->args.instructionPtr++;
                NEXT_OPCODE;
                
            BEGIN_OPCODE(WORDCHAR):
                if (stack.currentFrame->args.subjectPtr >= md.endSubject)
                    RRETURN_NO_MATCH;
                if (!isWordChar(*stack.currentFrame->args.subjectPtr++))
                    RRETURN_NO_MATCH;
                stack.currentFrame->args.instructionPtr++;
                NEXT_OPCODE;
                
            






                
            BEGIN_OPCODE(REF):
                stack.currentFrame->locals.offset = get2ByteValue(stack.currentFrame->args.instructionPtr + 1) << 1;               
                stack.currentFrame->args.instructionPtr += 3;                                 
                
                



                
                if (stack.currentFrame->locals.offset >= stack.currentFrame->args.offsetTop || md.offsetVector[stack.currentFrame->locals.offset] < 0)
                    stack.currentFrame->locals.length = 0;
                else
                    stack.currentFrame->locals.length = md.offsetVector[stack.currentFrame->locals.offset+1] - md.offsetVector[stack.currentFrame->locals.offset];
                
                
                
                switch (*stack.currentFrame->args.instructionPtr) {
                    case OP_CRSTAR:
                    case OP_CRMINSTAR:
                    case OP_CRPLUS:
                    case OP_CRMINPLUS:
                    case OP_CRQUERY:
                    case OP_CRMINQUERY:
                        repeatInformationFromInstructionOffset(*stack.currentFrame->args.instructionPtr++ - OP_CRSTAR, minimize, min, stack.currentFrame->locals.max);
                        break;
                        
                    case OP_CRRANGE:
                    case OP_CRMINRANGE:
                        minimize = (*stack.currentFrame->args.instructionPtr == OP_CRMINRANGE);
                        min = get2ByteValue(stack.currentFrame->args.instructionPtr + 1);
                        stack.currentFrame->locals.max = get2ByteValue(stack.currentFrame->args.instructionPtr + 3);
                        if (stack.currentFrame->locals.max == 0)
                            stack.currentFrame->locals.max = INT_MAX;
                        stack.currentFrame->args.instructionPtr += 5;
                        break;
                    
                    default:               
                        if (!matchRef(stack.currentFrame->locals.offset, stack.currentFrame->args.subjectPtr, stack.currentFrame->locals.length, md))
                            RRETURN_NO_MATCH;
                        stack.currentFrame->args.subjectPtr += stack.currentFrame->locals.length;
                        NEXT_OPCODE;
                }
                
                

                
                if (stack.currentFrame->locals.length == 0)
                    NEXT_OPCODE;
                
                
                
                for (int i = 1; i <= min; i++) {
                    if (!matchRef(stack.currentFrame->locals.offset, stack.currentFrame->args.subjectPtr, stack.currentFrame->locals.length, md))
                        RRETURN_NO_MATCH;
                    stack.currentFrame->args.subjectPtr += stack.currentFrame->locals.length;
                }
                
                

                
                if (min == stack.currentFrame->locals.max)
                    NEXT_OPCODE;
                
                
                
                if (minimize) {
                    for (stack.currentFrame->locals.fi = min;; stack.currentFrame->locals.fi++) {
                        RECURSIVE_MATCH(20, stack.currentFrame->args.instructionPtr, stack.currentFrame->args.bracketChain);
                        if (isMatch)
                            RRETURN;
                        if (stack.currentFrame->locals.fi >= stack.currentFrame->locals.max || !matchRef(stack.currentFrame->locals.offset, stack.currentFrame->args.subjectPtr, stack.currentFrame->locals.length, md))
                            RRETURN;
                        stack.currentFrame->args.subjectPtr += stack.currentFrame->locals.length;
                    }
                    
                }
                
                
                
                else {
                    stack.currentFrame->locals.subjectPtrAtStartOfInstruction = stack.currentFrame->args.subjectPtr;
                    for (int i = min; i < stack.currentFrame->locals.max; i++) {
                        if (!matchRef(stack.currentFrame->locals.offset, stack.currentFrame->args.subjectPtr, stack.currentFrame->locals.length, md))
                            break;
                        stack.currentFrame->args.subjectPtr += stack.currentFrame->locals.length;
                    }
                    while (stack.currentFrame->args.subjectPtr >= stack.currentFrame->locals.subjectPtrAtStartOfInstruction) {
                        RECURSIVE_MATCH(21, stack.currentFrame->args.instructionPtr, stack.currentFrame->args.bracketChain);
                        if (isMatch)
                            RRETURN;
                        stack.currentFrame->args.subjectPtr -= stack.currentFrame->locals.length;
                    }
                    RRETURN_NO_MATCH;
                }
                
                
            









                
            BEGIN_OPCODE(NCLASS):
            BEGIN_OPCODE(CLASS):
                stack.currentFrame->locals.data = stack.currentFrame->args.instructionPtr + 1;                
                stack.currentFrame->args.instructionPtr += 33;                     
                
                switch (*stack.currentFrame->args.instructionPtr) {
                    case OP_CRSTAR:
                    case OP_CRMINSTAR:
                    case OP_CRPLUS:
                    case OP_CRMINPLUS:
                    case OP_CRQUERY:
                    case OP_CRMINQUERY:
                        repeatInformationFromInstructionOffset(*stack.currentFrame->args.instructionPtr++ - OP_CRSTAR, minimize, min, stack.currentFrame->locals.max);
                        break;
                        
                    case OP_CRRANGE:
                    case OP_CRMINRANGE:
                        minimize = (*stack.currentFrame->args.instructionPtr == OP_CRMINRANGE);
                        min = get2ByteValue(stack.currentFrame->args.instructionPtr + 1);
                        stack.currentFrame->locals.max = get2ByteValue(stack.currentFrame->args.instructionPtr + 3);
                        if (stack.currentFrame->locals.max == 0)
                            stack.currentFrame->locals.max = INT_MAX;
                        stack.currentFrame->args.instructionPtr += 5;
                        break;
                        
                    default:               
                        min = stack.currentFrame->locals.max = 1;
                        break;
                }
                
                
                
                for (int i = 1; i <= min; i++) {
                    if (stack.currentFrame->args.subjectPtr >= md.endSubject)
                        RRETURN_NO_MATCH;
                    int c = *stack.currentFrame->args.subjectPtr++;
                    if (c > 255) {
                        if (stack.currentFrame->locals.data[-1] == OP_CLASS)
                            RRETURN_NO_MATCH;
                    } else {
                        if (!(stack.currentFrame->locals.data[c / 8] & (1 << (c & 7))))
                            RRETURN_NO_MATCH;
                    }
                }
                
                

                
                if (min == stack.currentFrame->locals.max)
                    NEXT_OPCODE;      
                
                

                if (minimize) {
                    for (stack.currentFrame->locals.fi = min;; stack.currentFrame->locals.fi++) {
                        RECURSIVE_MATCH(22, stack.currentFrame->args.instructionPtr, stack.currentFrame->args.bracketChain);
                        if (isMatch)
                            RRETURN;
                        if (stack.currentFrame->locals.fi >= stack.currentFrame->locals.max || stack.currentFrame->args.subjectPtr >= md.endSubject)
                            RRETURN;
                        int c = *stack.currentFrame->args.subjectPtr++;
                        if (c > 255) {
                            if (stack.currentFrame->locals.data[-1] == OP_CLASS)
                                RRETURN;
                        } else {
                            if ((stack.currentFrame->locals.data[c/8] & (1 << (c&7))) == 0)
                                RRETURN;
                        }
                    }
                    
                }
                
                else {
                    stack.currentFrame->locals.subjectPtrAtStartOfInstruction = stack.currentFrame->args.subjectPtr;
                    
                    for (int i = min; i < stack.currentFrame->locals.max; i++) {
                        if (stack.currentFrame->args.subjectPtr >= md.endSubject)
                            break;
                        int c = *stack.currentFrame->args.subjectPtr;
                        if (c > 255) {
                            if (stack.currentFrame->locals.data[-1] == OP_CLASS)
                                break;
                        } else {
                            if (!(stack.currentFrame->locals.data[c / 8] & (1 << (c & 7))))
                                break;
                        }
                        ++stack.currentFrame->args.subjectPtr;
                    }
                    for (;;) {
                        RECURSIVE_MATCH(24, stack.currentFrame->args.instructionPtr, stack.currentFrame->args.bracketChain);
                        if (isMatch)
                            RRETURN;
                        if (stack.currentFrame->args.subjectPtr-- == stack.currentFrame->locals.subjectPtrAtStartOfInstruction)
                            break;        
                    }
                    
                    RRETURN;
                }
                
                
            
                
            BEGIN_OPCODE(XCLASS):
                stack.currentFrame->locals.data = stack.currentFrame->args.instructionPtr + 1 + LINK_SIZE;                
                stack.currentFrame->args.instructionPtr += getLinkValue(stack.currentFrame->args.instructionPtr + 1);                      
                
                switch (*stack.currentFrame->args.instructionPtr) {
                    case OP_CRSTAR:
                    case OP_CRMINSTAR:
                    case OP_CRPLUS:
                    case OP_CRMINPLUS:
                    case OP_CRQUERY:
                    case OP_CRMINQUERY:
                        repeatInformationFromInstructionOffset(*stack.currentFrame->args.instructionPtr++ - OP_CRSTAR, minimize, min, stack.currentFrame->locals.max);
                        break;
                        
                    case OP_CRRANGE:
                    case OP_CRMINRANGE:
                        minimize = (*stack.currentFrame->args.instructionPtr == OP_CRMINRANGE);
                        min = get2ByteValue(stack.currentFrame->args.instructionPtr + 1);
                        stack.currentFrame->locals.max = get2ByteValue(stack.currentFrame->args.instructionPtr + 3);
                        if (stack.currentFrame->locals.max == 0)
                            stack.currentFrame->locals.max = INT_MAX;
                        stack.currentFrame->args.instructionPtr += 5;
                        break;
                        
                    default:               
                        min = stack.currentFrame->locals.max = 1;
                }
                
                
                
                for (int i = 1; i <= min; i++) {
                    if (stack.currentFrame->args.subjectPtr >= md.endSubject)
                        RRETURN_NO_MATCH;
                    int c = *stack.currentFrame->args.subjectPtr++;
                    if (!jsc_pcre_xclass(c, stack.currentFrame->locals.data))
                        RRETURN_NO_MATCH;
                }
                
                

                
                if (min == stack.currentFrame->locals.max)
                    NEXT_OPCODE;
                
                

                
                if (minimize) {
                    for (stack.currentFrame->locals.fi = min;; stack.currentFrame->locals.fi++) {
                        RECURSIVE_MATCH(26, stack.currentFrame->args.instructionPtr, stack.currentFrame->args.bracketChain);
                        if (isMatch)
                            RRETURN;
                        if (stack.currentFrame->locals.fi >= stack.currentFrame->locals.max || stack.currentFrame->args.subjectPtr >= md.endSubject)
                            RRETURN;
                        int c = *stack.currentFrame->args.subjectPtr++;
                        if (!jsc_pcre_xclass(c, stack.currentFrame->locals.data))
                            RRETURN;
                    }
                    
                }
                
                
                
                else {
                    stack.currentFrame->locals.subjectPtrAtStartOfInstruction = stack.currentFrame->args.subjectPtr;
                    for (int i = min; i < stack.currentFrame->locals.max; i++) {
                        if (stack.currentFrame->args.subjectPtr >= md.endSubject)
                            break;
                        int c = *stack.currentFrame->args.subjectPtr;
                        if (!jsc_pcre_xclass(c, stack.currentFrame->locals.data))
                            break;
                        ++stack.currentFrame->args.subjectPtr;
                    }
                    for(;;) {
                        RECURSIVE_MATCH(27, stack.currentFrame->args.instructionPtr, stack.currentFrame->args.bracketChain);
                        if (isMatch)
                            RRETURN;
                        if (stack.currentFrame->args.subjectPtr-- == stack.currentFrame->locals.subjectPtrAtStartOfInstruction)
                            break;        
                    }
                    RRETURN;
                }
                
                
                
            
                
            BEGIN_OPCODE(CHAR):
                stack.currentFrame->locals.length = 1;
                stack.currentFrame->args.instructionPtr++;
                getUTF8CharAndIncrementLength(stack.currentFrame->locals.fc, stack.currentFrame->args.instructionPtr, stack.currentFrame->locals.length);
                stack.currentFrame->args.instructionPtr += stack.currentFrame->locals.length;
                if (stack.currentFrame->args.subjectPtr >= md.endSubject)
                    RRETURN_NO_MATCH;
                if (stack.currentFrame->locals.fc != *stack.currentFrame->args.subjectPtr++)
                    RRETURN_NO_MATCH;
                NEXT_OPCODE;
                
            
                
            BEGIN_OPCODE(CHAR_IGNORING_CASE): {
                stack.currentFrame->locals.length = 1;
                stack.currentFrame->args.instructionPtr++;
                getUTF8CharAndIncrementLength(stack.currentFrame->locals.fc, stack.currentFrame->args.instructionPtr, stack.currentFrame->locals.length);
                stack.currentFrame->args.instructionPtr += stack.currentFrame->locals.length;
                if (stack.currentFrame->args.subjectPtr >= md.endSubject)
                    RRETURN_NO_MATCH;
                int dc = *stack.currentFrame->args.subjectPtr++;
                if (stack.currentFrame->locals.fc != dc && jsc_pcre_ucp_othercase(stack.currentFrame->locals.fc) != dc)
                    RRETURN_NO_MATCH;
                NEXT_OPCODE;
            }
                
            
                
            BEGIN_OPCODE(ASCII_CHAR):
                if (md.endSubject == stack.currentFrame->args.subjectPtr)
                    RRETURN_NO_MATCH;
                if (*stack.currentFrame->args.subjectPtr != stack.currentFrame->args.instructionPtr[1])
                    RRETURN_NO_MATCH;
                ++stack.currentFrame->args.subjectPtr;
                stack.currentFrame->args.instructionPtr += 2;
                NEXT_OPCODE;
                
            
                
            BEGIN_OPCODE(ASCII_LETTER_IGNORING_CASE):
                if (md.endSubject == stack.currentFrame->args.subjectPtr)
                    RRETURN_NO_MATCH;
                if ((*stack.currentFrame->args.subjectPtr | 0x20) != stack.currentFrame->args.instructionPtr[1])
                    RRETURN_NO_MATCH;
                ++stack.currentFrame->args.subjectPtr;
                stack.currentFrame->args.instructionPtr += 2;
                NEXT_OPCODE;
                
            
                
            BEGIN_OPCODE(EXACT):
                min = stack.currentFrame->locals.max = get2ByteValue(stack.currentFrame->args.instructionPtr + 1);
                minimize = false;
                stack.currentFrame->args.instructionPtr += 3;
                goto REPEATCHAR;
                
            BEGIN_OPCODE(UPTO):
            BEGIN_OPCODE(MINUPTO):
                min = 0;
                stack.currentFrame->locals.max = get2ByteValue(stack.currentFrame->args.instructionPtr + 1);
                minimize = *stack.currentFrame->args.instructionPtr == OP_MINUPTO;
                stack.currentFrame->args.instructionPtr += 3;
                goto REPEATCHAR;
                
            BEGIN_OPCODE(STAR):
            BEGIN_OPCODE(MINSTAR):
            BEGIN_OPCODE(PLUS):
            BEGIN_OPCODE(MINPLUS):
            BEGIN_OPCODE(QUERY):
            BEGIN_OPCODE(MINQUERY):
                repeatInformationFromInstructionOffset(*stack.currentFrame->args.instructionPtr++ - OP_STAR, minimize, min, stack.currentFrame->locals.max);
                
                


                
            REPEATCHAR:
                
                stack.currentFrame->locals.length = 1;
                getUTF8CharAndIncrementLength(stack.currentFrame->locals.fc, stack.currentFrame->args.instructionPtr, stack.currentFrame->locals.length);
                if (min * (stack.currentFrame->locals.fc > 0xFFFF ? 2 : 1) > md.endSubject - stack.currentFrame->args.subjectPtr)
                    RRETURN_NO_MATCH;
                stack.currentFrame->args.instructionPtr += stack.currentFrame->locals.length;
                
                if (stack.currentFrame->locals.fc <= 0xFFFF) {
                    othercase = md.ignoreCase ? jsc_pcre_ucp_othercase(stack.currentFrame->locals.fc) : -1;
                    
                    for (int i = 1; i <= min; i++) {
                        if (*stack.currentFrame->args.subjectPtr != stack.currentFrame->locals.fc && *stack.currentFrame->args.subjectPtr != othercase)
                            RRETURN_NO_MATCH;
                        ++stack.currentFrame->args.subjectPtr;
                    }
                    
                    if (min == stack.currentFrame->locals.max)
                        NEXT_OPCODE;
                    
                    if (minimize) {
                        stack.currentFrame->locals.repeatOthercase = othercase;
                        for (stack.currentFrame->locals.fi = min;; stack.currentFrame->locals.fi++) {
                            RECURSIVE_MATCH(28, stack.currentFrame->args.instructionPtr, stack.currentFrame->args.bracketChain);
                            if (isMatch)
                                RRETURN;
                            if (stack.currentFrame->locals.fi >= stack.currentFrame->locals.max || stack.currentFrame->args.subjectPtr >= md.endSubject)
                                RRETURN;
                            if (*stack.currentFrame->args.subjectPtr != stack.currentFrame->locals.fc && *stack.currentFrame->args.subjectPtr != stack.currentFrame->locals.repeatOthercase)
                                RRETURN;
                            ++stack.currentFrame->args.subjectPtr;
                        }
                        
                    } else {
                        stack.currentFrame->locals.subjectPtrAtStartOfInstruction = stack.currentFrame->args.subjectPtr;
                        for (int i = min; i < stack.currentFrame->locals.max; i++) {
                            if (stack.currentFrame->args.subjectPtr >= md.endSubject)
                                break;
                            if (*stack.currentFrame->args.subjectPtr != stack.currentFrame->locals.fc && *stack.currentFrame->args.subjectPtr != othercase)
                                break;
                            ++stack.currentFrame->args.subjectPtr;
                        }
                        while (stack.currentFrame->args.subjectPtr >= stack.currentFrame->locals.subjectPtrAtStartOfInstruction) {
                            RECURSIVE_MATCH(29, stack.currentFrame->args.instructionPtr, stack.currentFrame->args.bracketChain);
                            if (isMatch)
                                RRETURN;
                            --stack.currentFrame->args.subjectPtr;
                        }
                        RRETURN_NO_MATCH;
                    }
                    
                } else {
                    
                    
                    for (int i = 1; i <= min; i++) {
                        if (*stack.currentFrame->args.subjectPtr != stack.currentFrame->locals.fc)
                            RRETURN_NO_MATCH;
                        stack.currentFrame->args.subjectPtr += 2;
                    }
                    
                    if (min == stack.currentFrame->locals.max)
                        NEXT_OPCODE;
                    
                    if (minimize) {
                        for (stack.currentFrame->locals.fi = min;; stack.currentFrame->locals.fi++) {
                            RECURSIVE_MATCH(30, stack.currentFrame->args.instructionPtr, stack.currentFrame->args.bracketChain);
                            if (isMatch)
                                RRETURN;
                            if (stack.currentFrame->locals.fi >= stack.currentFrame->locals.max || stack.currentFrame->args.subjectPtr >= md.endSubject)
                                RRETURN;
                            if (*stack.currentFrame->args.subjectPtr != stack.currentFrame->locals.fc)
                                RRETURN;
                            stack.currentFrame->args.subjectPtr += 2;
                        }
                        
                    } else {
                        stack.currentFrame->locals.subjectPtrAtStartOfInstruction = stack.currentFrame->args.subjectPtr;
                        for (int i = min; i < stack.currentFrame->locals.max; i++) {
                            if (stack.currentFrame->args.subjectPtr > md.endSubject - 2)
                                break;
                            if (*stack.currentFrame->args.subjectPtr != stack.currentFrame->locals.fc)
                                break;
                            stack.currentFrame->args.subjectPtr += 2;
                        }
                        while (stack.currentFrame->args.subjectPtr >= stack.currentFrame->locals.subjectPtrAtStartOfInstruction) {
                            RECURSIVE_MATCH(31, stack.currentFrame->args.instructionPtr, stack.currentFrame->args.bracketChain);
                            if (isMatch)
                                RRETURN;
                            stack.currentFrame->args.subjectPtr -= 2;
                        }
                        RRETURN_NO_MATCH;
                    }
                    
                }
                
                
            
                
            BEGIN_OPCODE(NOT): {
                if (stack.currentFrame->args.subjectPtr >= md.endSubject)
                    RRETURN_NO_MATCH;
                int b = stack.currentFrame->args.instructionPtr[1];
                int c = *stack.currentFrame->args.subjectPtr++;
                stack.currentFrame->args.instructionPtr += 2;
                if (md.ignoreCase) {
                    if (c < 128)
                        c = toLowerCase(c);
                    if (toLowerCase(b) == c)
                        RRETURN_NO_MATCH;
                } else {
                    if (b == c)
                        RRETURN_NO_MATCH;
                }
                NEXT_OPCODE;
            }
                
            





                
            BEGIN_OPCODE(NOTEXACT):
                min = stack.currentFrame->locals.max = get2ByteValue(stack.currentFrame->args.instructionPtr + 1);
                minimize = false;
                stack.currentFrame->args.instructionPtr += 3;
                goto REPEATNOTCHAR;
                
            BEGIN_OPCODE(NOTUPTO):
            BEGIN_OPCODE(NOTMINUPTO):
                min = 0;
                stack.currentFrame->locals.max = get2ByteValue(stack.currentFrame->args.instructionPtr + 1);
                minimize = *stack.currentFrame->args.instructionPtr == OP_NOTMINUPTO;
                stack.currentFrame->args.instructionPtr += 3;
                goto REPEATNOTCHAR;
                
            BEGIN_OPCODE(NOTSTAR):
            BEGIN_OPCODE(NOTMINSTAR):
            BEGIN_OPCODE(NOTPLUS):
            BEGIN_OPCODE(NOTMINPLUS):
            BEGIN_OPCODE(NOTQUERY):
            BEGIN_OPCODE(NOTMINQUERY):
                repeatInformationFromInstructionOffset(*stack.currentFrame->args.instructionPtr++ - OP_NOTSTAR, minimize, min, stack.currentFrame->locals.max);
                
            


                
            REPEATNOTCHAR:
                if (min > md.endSubject - stack.currentFrame->args.subjectPtr)
                    RRETURN_NO_MATCH;
                stack.currentFrame->locals.fc = *stack.currentFrame->args.instructionPtr++;
                
                






                
                DPRINTF(("negative matching %c{%d,%d}\n", stack.currentFrame->locals.fc, min, stack.currentFrame->locals.max));
                
                if (md.ignoreCase) {
                    if (stack.currentFrame->locals.fc < 128)
                        stack.currentFrame->locals.fc = toLowerCase(stack.currentFrame->locals.fc);
                    
                    for (int i = 1; i <= min; i++) {
                        int d = *stack.currentFrame->args.subjectPtr++;
                        if (d < 128)
                            d = toLowerCase(d);
                        if (stack.currentFrame->locals.fc == d)
                            RRETURN_NO_MATCH;
                    }
                    
                    if (min == stack.currentFrame->locals.max)
                        NEXT_OPCODE;      
                    
                    if (minimize) {
                        for (stack.currentFrame->locals.fi = min;; stack.currentFrame->locals.fi++) {
                            RECURSIVE_MATCH(38, stack.currentFrame->args.instructionPtr, stack.currentFrame->args.bracketChain);
                            if (isMatch)
                                RRETURN;
                            int d = *stack.currentFrame->args.subjectPtr++;
                            if (d < 128)
                                d = toLowerCase(d);
                            if (stack.currentFrame->locals.fi >= stack.currentFrame->locals.max || stack.currentFrame->args.subjectPtr >= md.endSubject || stack.currentFrame->locals.fc == d)
                                RRETURN;
                        }
                        
                    }
                    
                    
                    
                    else {
                        stack.currentFrame->locals.subjectPtrAtStartOfInstruction = stack.currentFrame->args.subjectPtr;
                        
                        for (int i = min; i < stack.currentFrame->locals.max; i++) {
                            if (stack.currentFrame->args.subjectPtr >= md.endSubject)
                                break;
                            int d = *stack.currentFrame->args.subjectPtr;
                            if (d < 128)
                                d = toLowerCase(d);
                            if (stack.currentFrame->locals.fc == d)
                                break;
                            ++stack.currentFrame->args.subjectPtr;
                        }
                        for (;;) {
                            RECURSIVE_MATCH(40, stack.currentFrame->args.instructionPtr, stack.currentFrame->args.bracketChain);
                            if (isMatch)
                                RRETURN;
                            if (stack.currentFrame->args.subjectPtr-- == stack.currentFrame->locals.subjectPtrAtStartOfInstruction)
                                break;        
                        }
                        
                        RRETURN;
                    }
                    
                }
                
                
                
                else {
                    for (int i = 1; i <= min; i++) {
                        int d = *stack.currentFrame->args.subjectPtr++;
                        if (stack.currentFrame->locals.fc == d)
                            RRETURN_NO_MATCH;
                    }

                    if (min == stack.currentFrame->locals.max)
                        NEXT_OPCODE;
                    
                    if (minimize) {
                        for (stack.currentFrame->locals.fi = min;; stack.currentFrame->locals.fi++) {
                            RECURSIVE_MATCH(42, stack.currentFrame->args.instructionPtr, stack.currentFrame->args.bracketChain);
                            if (isMatch)
                                RRETURN;
                            int d = *stack.currentFrame->args.subjectPtr++;
                            if (stack.currentFrame->locals.fi >= stack.currentFrame->locals.max || stack.currentFrame->args.subjectPtr >= md.endSubject || stack.currentFrame->locals.fc == d)
                                RRETURN;
                        }
                        
                    }
                    
                    
                    
                    else {
                        stack.currentFrame->locals.subjectPtrAtStartOfInstruction = stack.currentFrame->args.subjectPtr;
                        
                        for (int i = min; i < stack.currentFrame->locals.max; i++) {
                            if (stack.currentFrame->args.subjectPtr >= md.endSubject)
                                break;
                            int d = *stack.currentFrame->args.subjectPtr;
                            if (stack.currentFrame->locals.fc == d)
                                break;
                            ++stack.currentFrame->args.subjectPtr;
                        }
                        for (;;) {
                            RECURSIVE_MATCH(44, stack.currentFrame->args.instructionPtr, stack.currentFrame->args.bracketChain);
                            if (isMatch)
                                RRETURN;
                            if (stack.currentFrame->args.subjectPtr-- == stack.currentFrame->locals.subjectPtrAtStartOfInstruction)
                                break;        
                        }

                        RRETURN;
                    }
                }
                
                
            


                
            BEGIN_OPCODE(TYPEEXACT):
                min = stack.currentFrame->locals.max = get2ByteValue(stack.currentFrame->args.instructionPtr + 1);
                minimize = true;
                stack.currentFrame->args.instructionPtr += 3;
                goto REPEATTYPE;
                
            BEGIN_OPCODE(TYPEUPTO):
            BEGIN_OPCODE(TYPEMINUPTO):
                min = 0;
                stack.currentFrame->locals.max = get2ByteValue(stack.currentFrame->args.instructionPtr + 1);
                minimize = *stack.currentFrame->args.instructionPtr == OP_TYPEMINUPTO;
                stack.currentFrame->args.instructionPtr += 3;
                goto REPEATTYPE;
                
            BEGIN_OPCODE(TYPESTAR):
            BEGIN_OPCODE(TYPEMINSTAR):
            BEGIN_OPCODE(TYPEPLUS):
            BEGIN_OPCODE(TYPEMINPLUS):
            BEGIN_OPCODE(TYPEQUERY):
            BEGIN_OPCODE(TYPEMINQUERY):
                repeatInformationFromInstructionOffset(*stack.currentFrame->args.instructionPtr++ - OP_TYPESTAR, minimize, min, stack.currentFrame->locals.max);
                
                


                
            REPEATTYPE:
                stack.currentFrame->locals.ctype = *stack.currentFrame->args.instructionPtr++;      
                
                



                
                if (min > md.endSubject - stack.currentFrame->args.subjectPtr)
                    RRETURN_NO_MATCH;
                if (min > 0) {
                    switch (stack.currentFrame->locals.ctype) {
                        case OP_NOT_NEWLINE:
                            for (int i = 1; i <= min; i++) {
                                if (isNewline(*stack.currentFrame->args.subjectPtr))
                                    RRETURN_NO_MATCH;
                                ++stack.currentFrame->args.subjectPtr;
                            }
                            break;
                            
                        case OP_NOT_DIGIT:
                            for (int i = 1; i <= min; i++) {
                                if (isASCIIDigit(*stack.currentFrame->args.subjectPtr))
                                    RRETURN_NO_MATCH;
                                ++stack.currentFrame->args.subjectPtr;
                            }
                            break;
                            
                        case OP_DIGIT:
                            for (int i = 1; i <= min; i++) {
                                if (!isASCIIDigit(*stack.currentFrame->args.subjectPtr))
                                    RRETURN_NO_MATCH;
                                ++stack.currentFrame->args.subjectPtr;
                            }
                            break;
                            
                        case OP_NOT_WHITESPACE:
                            for (int i = 1; i <= min; i++) {
                                if (isSpaceChar(*stack.currentFrame->args.subjectPtr))
                                    RRETURN_NO_MATCH;
                                ++stack.currentFrame->args.subjectPtr;
                            }
                            break;
                            
                        case OP_WHITESPACE:
                            for (int i = 1; i <= min; i++) {
                                if (!isSpaceChar(*stack.currentFrame->args.subjectPtr))
                                    RRETURN_NO_MATCH;
                                ++stack.currentFrame->args.subjectPtr;
                            }
                            break;
                            
                        case OP_NOT_WORDCHAR:
                            for (int i = 1; i <= min; i++) {
                                if (isWordChar(*stack.currentFrame->args.subjectPtr))
                                    RRETURN_NO_MATCH;
                                ++stack.currentFrame->args.subjectPtr;
                            }
                            break;
                            
                        case OP_WORDCHAR:
                            for (int i = 1; i <= min; i++) {
                                if (!isWordChar(*stack.currentFrame->args.subjectPtr))
                                    RRETURN_NO_MATCH;
                                ++stack.currentFrame->args.subjectPtr;
                            }
                            break;
                            
                        default:
                            JS_NOT_REACHED("Invalid character type.");
                            return matchError(JSRegExpErrorInternal, stack);
                    }  
                }
                
                
                
                if (min == stack.currentFrame->locals.max)
                    NEXT_OPCODE;    
                
                

                
                if (minimize) {
                    for (stack.currentFrame->locals.fi = min;; stack.currentFrame->locals.fi++) {
                        RECURSIVE_MATCH(48, stack.currentFrame->args.instructionPtr, stack.currentFrame->args.bracketChain);
                        if (isMatch)
                            RRETURN;
                        if (stack.currentFrame->locals.fi >= stack.currentFrame->locals.max || stack.currentFrame->args.subjectPtr >= md.endSubject)
                            RRETURN;
                        
                        int c = *stack.currentFrame->args.subjectPtr++;
                        switch (stack.currentFrame->locals.ctype) {
                            case OP_NOT_NEWLINE:
                                if (isNewline(c))
                                    RRETURN;
                                break;
                                
                            case OP_NOT_DIGIT:
                                if (isASCIIDigit(c))
                                    RRETURN;
                                break;
                                
                            case OP_DIGIT:
                                if (!isASCIIDigit(c))
                                    RRETURN;
                                break;
                                
                            case OP_NOT_WHITESPACE:
                                if (isSpaceChar(c))
                                    RRETURN;
                                break;
                                
                            case OP_WHITESPACE:
                                if (!isSpaceChar(c))
                                    RRETURN;
                                break;
                                
                            case OP_NOT_WORDCHAR:
                                if (isWordChar(c))
                                    RRETURN;
                                break;
                                
                            case OP_WORDCHAR:
                                if (!isWordChar(c))
                                    RRETURN;
                                break;
                                
                            default:
                                JS_NOT_REACHED("Invalid character type.");
                                return matchError(JSRegExpErrorInternal, stack);
                        }
                    }
                    
                }
                
                

                
                else {
                    stack.currentFrame->locals.subjectPtrAtStartOfInstruction = stack.currentFrame->args.subjectPtr;  
                    
                    switch (stack.currentFrame->locals.ctype) {
                        case OP_NOT_NEWLINE:
                            for (int i = min; i < stack.currentFrame->locals.max; i++) {
                                if (stack.currentFrame->args.subjectPtr >= md.endSubject || isNewline(*stack.currentFrame->args.subjectPtr))
                                    break;
                                stack.currentFrame->args.subjectPtr++;
                            }
                            break;
                            
                        case OP_NOT_DIGIT:
                            for (int i = min; i < stack.currentFrame->locals.max; i++) {
                                if (stack.currentFrame->args.subjectPtr >= md.endSubject)
                                    break;
                                int c = *stack.currentFrame->args.subjectPtr;
                                if (isASCIIDigit(c))
                                    break;
                                ++stack.currentFrame->args.subjectPtr;
                            }
                            break;
                            
                        case OP_DIGIT:
                            for (int i = min; i < stack.currentFrame->locals.max; i++) {
                                if (stack.currentFrame->args.subjectPtr >= md.endSubject)
                                    break;
                                int c = *stack.currentFrame->args.subjectPtr;
                                if (!isASCIIDigit(c))
                                    break;
                                ++stack.currentFrame->args.subjectPtr;
                            }
                            break;
                            
                        case OP_NOT_WHITESPACE:
                            for (int i = min; i < stack.currentFrame->locals.max; i++) {
                                if (stack.currentFrame->args.subjectPtr >= md.endSubject)
                                    break;
                                int c = *stack.currentFrame->args.subjectPtr;
                                if (isSpaceChar(c))
                                    break;
                                ++stack.currentFrame->args.subjectPtr;
                            }
                            break;
                            
                        case OP_WHITESPACE:
                            for (int i = min; i < stack.currentFrame->locals.max; i++) {
                                if (stack.currentFrame->args.subjectPtr >= md.endSubject)
                                    break;
                                int c = *stack.currentFrame->args.subjectPtr;
                                if (!isSpaceChar(c))
                                    break;
                                ++stack.currentFrame->args.subjectPtr;
                            }
                            break;
                            
                        case OP_NOT_WORDCHAR:
                            for (int i = min; i < stack.currentFrame->locals.max; i++) {
                                if (stack.currentFrame->args.subjectPtr >= md.endSubject)
                                    break;
                                int c = *stack.currentFrame->args.subjectPtr;
                                if (isWordChar(c))
                                    break;
                                ++stack.currentFrame->args.subjectPtr;
                            }
                            break;
                            
                        case OP_WORDCHAR:
                            for (int i = min; i < stack.currentFrame->locals.max; i++) {
                                if (stack.currentFrame->args.subjectPtr >= md.endSubject)
                                    break;
                                int c = *stack.currentFrame->args.subjectPtr;
                                if (!isWordChar(c))
                                    break;
                                ++stack.currentFrame->args.subjectPtr;
                            }
                            break;
                            
                        default:
                            JS_NOT_REACHED("Invalid character type.");
                            return matchError(JSRegExpErrorInternal, stack);
                    }
                    
                    
                    
                    for (;;) {
                        RECURSIVE_MATCH(52, stack.currentFrame->args.instructionPtr, stack.currentFrame->args.bracketChain);
                        if (isMatch)
                            RRETURN;
                        if (stack.currentFrame->args.subjectPtr-- == stack.currentFrame->locals.subjectPtrAtStartOfInstruction)
                            break;        
                    }
                    
                    
                    
                    RRETURN;
                }
                
                
            BEGIN_OPCODE(CRMINPLUS):
            BEGIN_OPCODE(CRMINQUERY):
            BEGIN_OPCODE(CRMINRANGE):
            BEGIN_OPCODE(CRMINSTAR):
            BEGIN_OPCODE(CRPLUS):
            BEGIN_OPCODE(CRQUERY):
            BEGIN_OPCODE(CRRANGE):
            BEGIN_OPCODE(CRSTAR):
                JS_NOT_REACHED("Invalid opcode.");
                return matchError(JSRegExpErrorInternal, stack);
                
#ifdef USE_COMPUTED_GOTO_FOR_MATCH_OPCODE_LOOP
            CAPTURING_BRACKET:
#else
            default:
#endif
                












                
                JS_ASSERT(*stack.currentFrame->args.instructionPtr > OP_BRA);
                
                LOCALS(number) = *stack.currentFrame->args.instructionPtr - OP_BRA;
                stack.currentFrame->extractBrackets(stack.currentFrame->args.instructionPtr);
                DPRINTF(("opening capturing bracket %d\n", stack.currentFrame->locals.number));
                
                

                
                if (stack.currentFrame->locals.number > EXTRACT_BASIC_MAX)
                    stack.currentFrame->locals.number = get2ByteValue(stack.currentFrame->args.instructionPtr + 4 + LINK_SIZE);
                stack.currentFrame->locals.offset = 2 * stack.currentFrame->locals.number;

                JS_ASSERT_IF(LOCALS(number), LOCALS(minBracket) <= LOCALS(number) && LOCALS(number) < LOCALS(limitBracket));
                
                if (stack.currentFrame->locals.offset < md.offsetMax) {
                    stack.currentFrame->locals.savedSubjectOffset = md.offsetVector[md.offsetEnd - stack.currentFrame->locals.number];
                    DPRINTF(("setting subject offset for bracket to %d\n", stack.currentFrame->args.subjectPtr - md.startSubject));
                    md.offsetVector[md.offsetEnd - stack.currentFrame->locals.number] = stack.currentFrame->args.subjectPtr - md.startSubject;
                    stack.currentFrame->locals.skipBytes = 3; 
                    
                    
                    stack.currentFrame->locals.minSatisfied = minSatNextBracket.readAndClear();
                    do {
                        

                        minSatisfied = stack.currentFrame->locals.minSatisfied;
                        RECURSIVE_MATCH_NEW_GROUP(1, stack.currentFrame->args.instructionPtr + stack.currentFrame->locals.skipBytes + LINK_SIZE, stack.currentFrame->args.bracketChain, minSatisfied);
                        if (isMatch)
                            RRETURN;
                        stack.currentFrame->locals.skipBytes = 1; 
                        stack.currentFrame->args.instructionPtr += getLinkValue(stack.currentFrame->args.instructionPtr + 1);
                    } while (*stack.currentFrame->args.instructionPtr == OP_ALT);
                    
                    DPRINTF(("bracket %d failed\n", stack.currentFrame->locals.number));
                    for (size_t i = LOCALS(minBracket); i < size_t(LOCALS(limitBracket)); ++i)
                        md.setOffsetPair(i, -1, -1);
                    DPRINTF(("restoring subject offset for bracket to %d\n", stack.currentFrame->locals.savedSubjectOffset));
                    md.offsetVector[md.offsetEnd - stack.currentFrame->locals.number] = stack.currentFrame->locals.savedSubjectOffset;
                    
                    RRETURN;
                }
                
                
                
                goto NON_CAPTURING_BRACKET;
        }
        
        


        
    } 
    
    JS_NOT_REACHED("Loop does not fallthru.");
    
#ifndef USE_COMPUTED_GOTO_FOR_MATCH_RECURSION
    
RRETURN_SWITCH:
    switch (stack.currentFrame->returnLocation) {
        case 0: goto RETURN;
        case 1: goto RRETURN_1;
        case 2: goto RRETURN_2;
        case 6: goto RRETURN_6;
        case 7: goto RRETURN_7;
        case 14: goto RRETURN_14;
        case 15: goto RRETURN_15;
        case 16: goto RRETURN_16;
        case 17: goto RRETURN_17;
        case 18: goto RRETURN_18;
        case 19: goto RRETURN_19;
        case 20: goto RRETURN_20;
        case 21: goto RRETURN_21;
        case 22: goto RRETURN_22;
        case 24: goto RRETURN_24;
        case 26: goto RRETURN_26;
        case 27: goto RRETURN_27;
        case 28: goto RRETURN_28;
        case 29: goto RRETURN_29;
        case 30: goto RRETURN_30;
        case 31: goto RRETURN_31;
        case 38: goto RRETURN_38;
        case 40: goto RRETURN_40;
        case 42: goto RRETURN_42;
        case 44: goto RRETURN_44;
        case 48: goto RRETURN_48;
        case 52: goto RRETURN_52;
    }
    
    JS_NOT_REACHED("Bad computed return location.");
    return matchError(JSRegExpErrorInternal, stack);
    
#endif
    
RETURN:
    return isMatch;
}


























static void tryFirstByteOptimization(const UChar*& subjectPtr, const UChar* endSubject, int firstByte, bool firstByteIsCaseless, bool useMultiLineFirstCharOptimization, const UChar* originalSubjectStart)
{
    
    
    if (firstByte >= 0) {
        UChar firstChar = firstByte;
        if (firstByteIsCaseless)
            while (subjectPtr < endSubject) {
                int c = *subjectPtr;
                if (c > 127)
                    break;
                if (toLowerCase(c) == firstChar)
                    break;
                subjectPtr++;
            }
        else {
            while (subjectPtr < endSubject && *subjectPtr != firstChar)
                subjectPtr++;
        }
    } else if (useMultiLineFirstCharOptimization) {
        
        
        if (subjectPtr > originalSubjectStart) {
            while (subjectPtr < endSubject && !isNewline(subjectPtr[-1]))
                subjectPtr++;
        }
    }
}

static bool tryRequiredByteOptimization(const UChar*& subjectPtr, const UChar* endSubject, int reqByte, int reqByte2, bool reqByteIsCaseless, bool hasFirstByte, const UChar*& reqBytePtr)
{
    













    if (reqByte >= 0 && endSubject - subjectPtr < REQ_BYTE_MAX) {
        const UChar* p = subjectPtr + (hasFirstByte ? 1 : 0);

        


        if (p > reqBytePtr) {
            if (reqByteIsCaseless) {
                while (p < endSubject) {
                    int pp = *p++;
                    if (pp == reqByte || pp == reqByte2) {
                        p--;
                        break;
                    }
                }
            } else {
                while (p < endSubject) {
                    if (*p++ == reqByte) {
                        p--;
                        break;
                    }
                }
            }

            

            if (p >= endSubject)
                return true;

            



            reqBytePtr = p;
        }
    }
    return false;
}

int jsRegExpExecute(JSContext *cx, const JSRegExp* re,
                    const UChar* subject, int length, int start_offset, int* offsets,
                    int offsetCount)
{
    JS_ASSERT(re);
    JS_ASSERT(subject || !length);
    JS_ASSERT(offsetCount >= 0);
    JS_ASSERT(offsets || offsetCount == 0);

    MatchData matchBlock;
    matchBlock.startSubject = subject;
    matchBlock.endSubject = matchBlock.startSubject + length;
    const UChar* endSubject = matchBlock.endSubject;
    
    matchBlock.multiline = (re->options & MatchAcrossMultipleLinesOption);
    matchBlock.ignoreCase = (re->options & IgnoreCaseOption);
    
    
    int ocount = offsetCount - (offsetCount % 3);
    
    matchBlock.offsetVector = offsets;
    matchBlock.offsetEnd = ocount;
    matchBlock.offsetMax = (2*ocount)/3;
    matchBlock.offsetOverflow = false;
    
    


    
    int resetCount = 2 + re->topBracket * 2;
    if (resetCount > offsetCount)
        resetCount = ocount;
    
    


    
    if (matchBlock.offsetVector) {
        int* iptr = matchBlock.offsetVector + ocount;
        int* iend = iptr - resetCount/2 + 1;
        while (--iptr >= iend)
            *iptr = -1;
    }
    
    




    
    bool firstByteIsCaseless = false;
    int firstByte = -1;
    if (re->options & UseFirstByteOptimizationOption) {
        firstByte = re->firstByte & 255;
        if ((firstByteIsCaseless = (re->firstByte & REQ_IGNORE_CASE)))
            firstByte = toLowerCase(firstByte);
    }
    
    

    
    bool reqByteIsCaseless = false;
    int reqByte = -1;
    int reqByte2 = -1;
    if (re->options & UseRequiredByteOptimizationOption) {
        reqByte = re->reqByte & 255;
        reqByteIsCaseless = (re->reqByte & REQ_IGNORE_CASE);
        reqByte2 = flipCase(reqByte);
    }
    
    

    
    const UChar* startMatch = subject + start_offset;
    const UChar* reqBytePtr = startMatch - 1;
    bool useMultiLineFirstCharOptimization = re->options & UseMultiLineFirstByteOptimizationOption;
    
    do {
        
        if (matchBlock.offsetVector) {
            int* iptr = matchBlock.offsetVector;
            int* iend = iptr + resetCount;
            while (iptr < iend)
                *iptr++ = -1;
        }
        
        tryFirstByteOptimization(startMatch, endSubject, firstByte, firstByteIsCaseless, useMultiLineFirstCharOptimization, matchBlock.startSubject + start_offset);
        if (tryRequiredByteOptimization(startMatch, endSubject, reqByte, reqByte2, reqByteIsCaseless, firstByte >= 0, reqBytePtr))
            break;
                
        





        
        
        const unsigned char* start_code = (const unsigned char*)(re + 1);
        
        int returnCode = match(&cx->regExpPool, startMatch, start_code, 2, matchBlock);
        
        

        if (returnCode == 0) {
            startMatch++;
            continue;
        }

        if (returnCode != 1) {
            JS_ASSERT(returnCode == JSRegExpErrorHitLimit);
            DPRINTF((">>>> error: returning %d\n", returnCode));
            return returnCode;
        }
        
        
        
        returnCode = matchBlock.offsetOverflow ? 0 : matchBlock.endOffsetTop / 2;
        
        if (offsetCount < 2)
            returnCode = 0;
        else {
            offsets[0] = startMatch - matchBlock.startSubject;
            offsets[1] = matchBlock.endMatchPtr - matchBlock.startSubject;
        }
        
        JS_ASSERT(returnCode >= 0);
        DPRINTF((">>>> returning %d\n", returnCode));
        return returnCode;
    } while (!(re->options & IsAnchoredOption) && startMatch <= endSubject);
    
    DPRINTF((">>>> returning PCRE_ERROR_NOMATCH\n"));
    return JSRegExpErrorNoMatch;
}
