







































#ifndef RegExpStatics_inl_h__
#define RegExpStatics_inl_h__

#include "RegExpStatics.h"

#include "vm/String-inl.h"

namespace js {

inline
RegExpStatics::RegExpStatics()
  : bufferLink(NULL),
    copied(false)
{
    clear();
}

inline bool
RegExpStatics::createDependent(JSContext *cx, size_t start, size_t end, Value *out) const 
{
    JS_ASSERT(start <= end);
    JS_ASSERT(end <= matchPairsInput->length());
    JSString *str = js_NewDependentString(cx, matchPairsInput, start, end - start);
    if (!str)
        return false;
    *out = StringValue(str);
    return true;
}

inline bool
RegExpStatics::createPendingInput(JSContext *cx, Value *out) const
{
    out->setString(pendingInput ? pendingInput.get() : cx->runtime->emptyString);
    return true;
}

inline bool
RegExpStatics::makeMatch(JSContext *cx, size_t checkValidIndex, size_t pairNum, Value *out) const
{
    if (checkValidIndex / 2 >= pairCount() || matchPairs[checkValidIndex] < 0) {
        out->setString(cx->runtime->emptyString);
        return true;
    }
    return createDependent(cx, get(pairNum, 0), get(pairNum, 1), out);
}

inline bool
RegExpStatics::createLastParen(JSContext *cx, Value *out) const
{
    if (pairCount() <= 1) {
        out->setString(cx->runtime->emptyString);
        return true;
    }
    size_t num = pairCount() - 1;
    int start = get(num, 0);
    int end = get(num, 1);
    if (start == -1) {
        out->setString(cx->runtime->emptyString);
        return true;
    }
    JS_ASSERT(start >= 0 && end >= 0);
    JS_ASSERT(end >= start);
    return createDependent(cx, start, end, out);
}

inline bool
RegExpStatics::createLeftContext(JSContext *cx, Value *out) const
{
    if (!pairCount()) {
        out->setString(cx->runtime->emptyString);
        return true;
    }
    if (matchPairs[0] < 0) {
        *out = UndefinedValue();
        return true;
    }
    return createDependent(cx, 0, matchPairs[0], out);
}

inline bool
RegExpStatics::createRightContext(JSContext *cx, Value *out) const
{
    if (!pairCount()) {
        out->setString(cx->runtime->emptyString);
        return true;
    }
    if (matchPairs[1] < 0) {
        *out = UndefinedValue();
        return true;
    }
    return createDependent(cx, matchPairs[1], matchPairsInput->length(), out);
}

inline void
RegExpStatics::getParen(size_t pairNum, JSSubString *out) const
{
    checkParenNum(pairNum);
    if (!pairIsPresent(pairNum)) {
        *out = js_EmptySubString;
        return;
    }
    out->chars = matchPairsInput->chars() + get(pairNum, 0);
    out->length = getParenLength(pairNum);
}

inline void
RegExpStatics::getLastMatch(JSSubString *out) const
{
    if (!pairCount()) {
        *out = js_EmptySubString;
        return;
    }
    JS_ASSERT(matchPairsInput);
    out->chars = matchPairsInput->chars() + get(0, 0);
    JS_ASSERT(get(0, 1) >= get(0, 0));
    out->length = get(0, 1) - get(0, 0);
}

inline void
RegExpStatics::getLastParen(JSSubString *out) const
{
    size_t pc = pairCount();
    
    if (pc <= 1) {
        *out = js_EmptySubString;
        return;
    }
    getParen(pc - 1, out);
}

inline void
RegExpStatics::getLeftContext(JSSubString *out) const
{
    if (!pairCount()) {
        *out = js_EmptySubString;
        return;
    }
    out->chars = matchPairsInput->chars();
    out->length = get(0, 0);
}

inline void
RegExpStatics::getRightContext(JSSubString *out) const
{
    if (!pairCount()) {
        *out = js_EmptySubString;
        return;
    }
    out->chars = matchPairsInput->chars() + get(0, 1);
    JS_ASSERT(get(0, 1) <= int(matchPairsInput->length()));
    out->length = matchPairsInput->length() - get(0, 1);
}

inline void
RegExpStatics::copyTo(RegExpStatics &dst)
{
    dst.matchPairs.clear();
    
    dst.matchPairs.infallibleAppend(matchPairs);
    dst.matchPairsInput = matchPairsInput;
    dst.pendingInput = pendingInput;
    dst.flags = flags;
}

inline void
RegExpStatics::aboutToWrite()
{
    if (bufferLink && !bufferLink->copied) {
        copyTo(*bufferLink);
        bufferLink->copied = true;
    }
}

inline void
RegExpStatics::restore()
{
    if (bufferLink->copied)
        bufferLink->copyTo(*this);
    bufferLink = bufferLink->bufferLink;
}

inline bool
RegExpStatics::updateFromMatchPairs(JSContext *cx, JSLinearString *input, MatchPairs *newPairs)
{
    JS_ASSERT(input);
    aboutToWrite();
    BarrieredSetPair<JSString, JSLinearString>(cx->compartment,
                                               pendingInput, input,
                                               matchPairsInput, input);

    if (!matchPairs.resizeUninitialized(2 * newPairs->pairCount())) {
        js_ReportOutOfMemory(cx);
        return false;
    }

    for (size_t i = 0; i < newPairs->pairCount(); ++i) {
        matchPairs[2 * i] = newPairs->pair(i).start;
        matchPairs[2 * i + 1] = newPairs->pair(i).limit;
    }

    return true;
}

inline void
RegExpStatics::clear()
{
    aboutToWrite();
    flags = RegExpFlag(0);
    pendingInput = NULL;
    matchPairsInput = NULL;
    matchPairs.clear();
}

inline void
RegExpStatics::setPendingInput(JSString *newInput)
{
    aboutToWrite();
    pendingInput = newInput;
}

PreserveRegExpStatics::~PreserveRegExpStatics()
{
    original->restore();
}

inline void
RegExpStatics::setMultiline(JSContext *cx, bool enabled)
{
    aboutToWrite();
    if (enabled) {
        flags = RegExpFlag(flags | MultilineFlag);
        markFlagsSet(cx);
    } else {
        flags = RegExpFlag(flags & ~MultilineFlag);
    }
}

inline void
RegExpStatics::markFlagsSet(JSContext *cx)
{
    







    GlobalObject *global = GetGlobalForScopeChain(cx);
    JS_ASSERT(this == global->getRegExpStatics());

    types::MarkTypeObjectFlags(cx, global, types::OBJECT_FLAG_REGEXP_FLAGS_SET);
}

inline void
RegExpStatics::reset(JSContext *cx, JSString *newInput, bool newMultiline)
{
    aboutToWrite();
    clear();
    pendingInput = newInput;
    setMultiline(cx, newMultiline);
    checkInvariants();
}

} 

#endif
