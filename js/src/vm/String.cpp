





#include "vm/String-inl.h"

#include "mozilla/MathAlgorithms.h"
#include "mozilla/MemoryReporting.h"
#include "mozilla/PodOperations.h"
#include "mozilla/RangedPtr.h"
#include "mozilla/TypeTraits.h"

#include "gc/Marking.h"

#include "jscntxtinlines.h"
#include "jscompartmentinlines.h"

using namespace js;

using mozilla::IsSame;
using mozilla::PodCopy;
using mozilla::RangedPtr;
using mozilla::RoundUpPow2;

using JS::AutoCheckCannotGC;

size_t
JSString::sizeOfExcludingThis(mozilla::MallocSizeOf mallocSizeOf)
{
    
    if (isRope())
        return 0;

    JS_ASSERT(isLinear());

    
    if (isDependent())
        return 0;

    JS_ASSERT(isFlat());

    
    if (isExtensible()) {
        JSExtensibleString &extensible = asExtensible();
        return extensible.hasLatin1Chars()
               ? mallocSizeOf(extensible.rawLatin1Chars())
               : mallocSizeOf(extensible.rawTwoByteChars());
    }

    
    if (isExternal())
        return 0;

    
    if (isInline())
        return 0;

    
    
    
    JSFlatString &flat = asFlat();
    return flat.hasLatin1Chars()
           ? mallocSizeOf(flat.rawLatin1Chars())
           : mallocSizeOf(flat.rawTwoByteChars());
}

#ifdef DEBUG

template <typename CharT>
 void
JSString::dumpChars(const CharT *s, size_t n, FILE *fp)
{
    if (n == SIZE_MAX) {
        n = 0;
        while (s[n])
            n++;
    }

    fputc('"', fp);
    for (size_t i = 0; i < n; i++) {
        char16_t c = s[i];
        if (c == '\n')
            fprintf(fp, "\\n");
        else if (c == '\t')
            fprintf(fp, "\\t");
        else if (c >= 32 && c < 127)
            fputc(s[i], fp);
        else if (c <= 255)
            fprintf(fp, "\\x%02x", unsigned(c));
        else
            fprintf(fp, "\\u%04x", unsigned(c));
    }
    fputc('"', fp);
}

template void
JSString::dumpChars(const Latin1Char *s, size_t n, FILE *fp);

template void
JSString::dumpChars(const char16_t *s, size_t n, FILE *fp);

void
JSString::dumpCharsNoNewline(FILE *fp)
{
    if (JSLinearString *linear = ensureLinear(nullptr)) {
        AutoCheckCannotGC nogc;
        if (hasLatin1Chars())
            dumpChars(linear->latin1Chars(nogc), length(), fp);
        else
            dumpChars(linear->twoByteChars(nogc), length(), fp);
    } else {
        fprintf(fp, "(oom in JSString::dumpCharsNoNewline)");
    }
}

void
JSString::dump()
{
    if (JSLinearString *linear = ensureLinear(nullptr)) {
        AutoCheckCannotGC nogc;
        if (hasLatin1Chars()) {
            const Latin1Char *chars = linear->latin1Chars(nogc);
            fprintf(stderr, "JSString* (%p) = Latin1Char * (%p) = ", (void *) this,
                    (void *) chars);
            dumpChars(chars, length(), stderr);
        } else {
            const char16_t *chars = linear->twoByteChars(nogc);
            fprintf(stderr, "JSString* (%p) = char16_t * (%p) = ", (void *) this,
                    (void *) chars);
            dumpChars(chars, length(), stderr);
        }
    } else {
        fprintf(stderr, "(oom in JSString::dump)");
    }
    fputc('\n', stderr);
}

bool
JSString::equals(const char *s)
{
    JSLinearString *linear = ensureLinear(nullptr);
    if (!linear) {
        fprintf(stderr, "OOM in JSString::equals!\n");
        return false;
    }

    return StringEqualsAscii(linear, s);
}
#endif 

template <typename CharT>
static MOZ_ALWAYS_INLINE bool
AllocChars(JSString *str, size_t length, CharT **chars, size_t *capacity)
{
    




    size_t numChars = length + 1;

    




    static const size_t DOUBLING_MAX = 1024 * 1024;
    numChars = numChars > DOUBLING_MAX ? numChars + (numChars / 8) : RoundUpPow2(numChars);

    
    *capacity = numChars - 1;

    JS_STATIC_ASSERT(JSString::MAX_LENGTH * sizeof(CharT) < UINT32_MAX);
    *chars = str->pod_malloc<CharT>(numChars);
    return *chars != nullptr;
}

bool
JSRope::copyLatin1CharsZ(ThreadSafeContext *cx, ScopedJSFreePtr<Latin1Char> &out) const
{
    return copyCharsInternal<Latin1Char>(cx, out, true);
}

bool
JSRope::copyTwoByteCharsZ(ThreadSafeContext *cx, ScopedJSFreePtr<char16_t> &out) const
{
    return copyCharsInternal<char16_t>(cx, out, true);
}

bool
JSRope::copyLatin1Chars(ThreadSafeContext *cx, ScopedJSFreePtr<Latin1Char> &out) const
{
    return copyCharsInternal<Latin1Char>(cx, out, false);
}

bool
JSRope::copyTwoByteChars(ThreadSafeContext *cx, ScopedJSFreePtr<char16_t> &out) const
{
    return copyCharsInternal<char16_t>(cx, out, false);
}

template <typename CharT>
bool
JSRope::copyCharsInternal(ThreadSafeContext *cx, ScopedJSFreePtr<CharT> &out,
                          bool nullTerminate) const
{
    




    size_t n = length();
    if (cx)
        out.reset(cx->pod_malloc<CharT>(n + 1));
    else
        out.reset(js_pod_malloc<CharT>(n + 1));

    if (!out)
        return false;

    Vector<const JSString *, 8, SystemAllocPolicy> nodeStack;
    const JSString *str = this;
    CharT *pos = out;
    while (true) {
        if (str->isRope()) {
            if (!nodeStack.append(str->asRope().rightChild()))
                return false;
            str = str->asRope().leftChild();
        } else {
            CopyChars(pos, str->asLinear());
            pos += str->length();
            if (nodeStack.empty())
                break;
            str = nodeStack.popCopy();
        }
    }

    JS_ASSERT(pos == out + n);

    if (nullTerminate)
        out[n] = 0;

    return true;
}

namespace js {

template <>
void
CopyChars(char16_t *dest, const JSLinearString &str)
{
    AutoCheckCannotGC nogc;
    if (str.hasTwoByteChars())
        PodCopy(dest, str.twoByteChars(nogc), str.length());
    else
        CopyAndInflateChars(dest, str.latin1Chars(nogc), str.length());
}

template <>
void
CopyChars(Latin1Char *dest, const JSLinearString &str)
{
    AutoCheckCannotGC nogc;
    if (str.hasLatin1Chars()) {
        PodCopy(dest, str.latin1Chars(nogc), str.length());
    } else {
        







        size_t len = str.length();
        const char16_t *chars = str.twoByteChars(nogc);
        for (size_t i = 0; i < len; i++) {
            MOZ_ASSERT(chars[i] <= JSString::MAX_LATIN1_CHAR);
            dest[i] = chars[i];
        }
    }
}

} 

template<JSRope::UsingBarrier b, typename CharT>
JSFlatString *
JSRope::flattenInternal(ExclusiveContext *maybecx)
{
    






























    const size_t wholeLength = length();
    size_t wholeCapacity;
    CharT *wholeChars;
    JSString *str = this;
    CharT *pos;

    



    static const uintptr_t Tag_Mask = 0x3;
    static const uintptr_t Tag_FinishNode = 0x0;
    static const uintptr_t Tag_VisitRightChild = 0x1;

    AutoCheckCannotGC nogc;

    
    JSRope *leftMostRope = this;
    while (leftMostRope->leftChild()->isRope())
        leftMostRope = &leftMostRope->leftChild()->asRope();

    if (leftMostRope->leftChild()->isExtensible()) {
        JSExtensibleString &left = leftMostRope->leftChild()->asExtensible();
        size_t capacity = left.capacity();
        if (capacity >= wholeLength && left.hasTwoByteChars() == IsSame<CharT, char16_t>::value) {
            



            JS_ASSERT(str->isRope());
            while (str != leftMostRope) {
                if (b == WithIncrementalBarrier) {
                    JSString::writeBarrierPre(str->d.s.u2.left);
                    JSString::writeBarrierPre(str->d.s.u3.right);
                }
                JSString *child = str->d.s.u2.left;
                JS_ASSERT(child->isRope());
                str->setNonInlineChars(left.nonInlineChars<CharT>(nogc));
                child->d.u1.flattenData = uintptr_t(str) | Tag_VisitRightChild;
                str = child;
            }
            if (b == WithIncrementalBarrier) {
                JSString::writeBarrierPre(str->d.s.u2.left);
                JSString::writeBarrierPre(str->d.s.u3.right);
            }
            str->setNonInlineChars(left.nonInlineChars<CharT>(nogc));
            wholeCapacity = capacity;
            wholeChars = const_cast<CharT *>(left.nonInlineChars<CharT>(nogc));
            pos = wholeChars + left.d.u1.length;
            JS_STATIC_ASSERT(!(EXTENSIBLE_FLAGS & DEPENDENT_FLAGS));
            left.d.u1.flags ^= (EXTENSIBLE_FLAGS | DEPENDENT_FLAGS);
            left.d.s.u3.base = (JSLinearString *)this;  
            StringWriteBarrierPostRemove(maybecx, &left.d.s.u2.left);
            StringWriteBarrierPost(maybecx, (JSString **)&left.d.s.u3.base);
            goto visit_right_child;
        }
    }

    if (!AllocChars(this, wholeLength, &wholeChars, &wholeCapacity))
        return nullptr;

    pos = wholeChars;
    first_visit_node: {
        if (b == WithIncrementalBarrier) {
            JSString::writeBarrierPre(str->d.s.u2.left);
            JSString::writeBarrierPre(str->d.s.u3.right);
        }

        JSString &left = *str->d.s.u2.left;
        str->setNonInlineChars(pos);
        StringWriteBarrierPostRemove(maybecx, &str->d.s.u2.left);
        if (left.isRope()) {
            
            left.d.u1.flattenData = uintptr_t(str) | Tag_VisitRightChild;
            str = &left;
            goto first_visit_node;
        }
        CopyChars(pos, left.asLinear());
        pos += left.length();
    }
    visit_right_child: {
        JSString &right = *str->d.s.u3.right;
        if (right.isRope()) {
            
            right.d.u1.flattenData = uintptr_t(str) | Tag_FinishNode;
            str = &right;
            goto first_visit_node;
        }
        CopyChars(pos, right.asLinear());
        pos += right.length();
    }
    finish_node: {
        if (str == this) {
            JS_ASSERT(pos == wholeChars + wholeLength);
            *pos = '\0';
            str->d.u1.length = wholeLength;
            if (IsSame<CharT, char16_t>::value)
                str->d.u1.flags = EXTENSIBLE_FLAGS;
            else
                str->d.u1.flags = EXTENSIBLE_FLAGS | LATIN1_CHARS_BIT;
            str->setNonInlineChars(wholeChars);
            str->d.s.u3.capacity = wholeCapacity;
            StringWriteBarrierPostRemove(maybecx, &str->d.s.u2.left);
            StringWriteBarrierPostRemove(maybecx, &str->d.s.u3.right);
            return &this->asFlat();
        }
        uintptr_t flattenData = str->d.u1.flattenData;
        if (IsSame<CharT, char16_t>::value)
            str->d.u1.flags = DEPENDENT_FLAGS;
        else
            str->d.u1.flags = DEPENDENT_FLAGS | LATIN1_CHARS_BIT;
        str->d.u1.length = pos - str->asLinear().nonInlineChars<CharT>(nogc);
        str->d.s.u3.base = (JSLinearString *)this;       
        StringWriteBarrierPost(maybecx, (JSString **)&str->d.s.u3.base);
        str = (JSString *)(flattenData & ~Tag_Mask);
        if ((flattenData & Tag_Mask) == Tag_VisitRightChild)
            goto visit_right_child;
        JS_ASSERT((flattenData & Tag_Mask) == Tag_FinishNode);
        goto finish_node;
    }
}

template<JSRope::UsingBarrier b>
JSFlatString *
JSRope::flattenInternal(ExclusiveContext *maybecx)
{
    if (hasTwoByteChars())
        return flattenInternal<b, char16_t>(maybecx);
    return flattenInternal<b, Latin1Char>(maybecx);
}

JSFlatString *
JSRope::flatten(ExclusiveContext *maybecx)
{
#ifdef JSGC_INCREMENTAL
    if (zone()->needsIncrementalBarrier())
        return flattenInternal<WithIncrementalBarrier>(maybecx);
#endif
    return flattenInternal<NoBarrier>(maybecx);
}

template <AllowGC allowGC>
JSString *
js::ConcatStrings(ThreadSafeContext *cx,
                  typename MaybeRooted<JSString*, allowGC>::HandleType left,
                  typename MaybeRooted<JSString*, allowGC>::HandleType right)
{
    JS_ASSERT_IF(!left->isAtom(), cx->isInsideCurrentZone(left));
    JS_ASSERT_IF(!right->isAtom(), cx->isInsideCurrentZone(right));

    size_t leftLen = left->length();
    if (leftLen == 0)
        return right;

    size_t rightLen = right->length();
    if (rightLen == 0)
        return left;

    size_t wholeLength = leftLen + rightLen;
    if (!JSString::validateLength(cx, wholeLength))
        return nullptr;

    bool isLatin1 = left->hasLatin1Chars() && right->hasLatin1Chars();
    bool canUseFatInline = isLatin1
                           ? JSFatInlineString::latin1LengthFits(wholeLength)
                           : JSFatInlineString::twoByteLengthFits(wholeLength);
    if (canUseFatInline && cx->isJSContext()) {
        JSFatInlineString *str = NewGCFatInlineString<allowGC>(cx);
        if (!str)
            return nullptr;

        AutoCheckCannotGC nogc;
        ScopedThreadSafeStringInspector leftInspector(left);
        ScopedThreadSafeStringInspector rightInspector(right);
        if (!leftInspector.ensureChars(cx, nogc) || !rightInspector.ensureChars(cx, nogc))
            return nullptr;

        if (isLatin1) {
            Latin1Char *buf = str->initLatin1(wholeLength);
            PodCopy(buf, leftInspector.latin1Chars(), leftLen);
            PodCopy(buf + leftLen, rightInspector.latin1Chars(), rightLen);
            buf[wholeLength] = 0;
        } else {
            char16_t *buf = str->initTwoByte(wholeLength);
            if (leftInspector.hasTwoByteChars())
                PodCopy(buf, leftInspector.twoByteChars(), leftLen);
            else
                CopyAndInflateChars(buf, leftInspector.latin1Chars(), leftLen);
            if (rightInspector.hasTwoByteChars())
                PodCopy(buf + leftLen, rightInspector.twoByteChars(), rightLen);
            else
                CopyAndInflateChars(buf + leftLen, rightInspector.latin1Chars(), rightLen);
            buf[wholeLength] = 0;
        }

        return str;
    }

    return JSRope::new_<allowGC>(cx, left, right, wholeLength);
}

template JSString *
js::ConcatStrings<CanGC>(ThreadSafeContext *cx, HandleString left, HandleString right);

template JSString *
js::ConcatStrings<NoGC>(ThreadSafeContext *cx, JSString *left, JSString *right);

template <typename CharT>
JSFlatString *
JSDependentString::undependInternal(ExclusiveContext *cx)
{
    




    JSString::writeBarrierPre(base());

    size_t n = length();
    CharT *s = cx->pod_malloc<CharT>(n + 1);
    if (!s)
        return nullptr;

    AutoCheckCannotGC nogc;
    PodCopy(s, nonInlineChars<CharT>(nogc), n);
    s[n] = '\0';
    setNonInlineChars<CharT>(s);

    



    if (IsSame<CharT, Latin1Char>::value)
        d.u1.flags = UNDEPENDED_FLAGS | LATIN1_CHARS_BIT;
    else
        d.u1.flags = UNDEPENDED_FLAGS;

    return &this->asFlat();
}

JSFlatString *
JSDependentString::undepend(ExclusiveContext *cx)
{
    JS_ASSERT(JSString::isDependent());
    return hasLatin1Chars()
           ? undependInternal<Latin1Char>(cx)
           : undependInternal<char16_t>(cx);
}

template <typename CharT>
 bool
JSFlatString::isIndexSlow(const CharT *s, size_t length, uint32_t *indexp)
{
    CharT ch = *s;

    if (!JS7_ISDEC(ch))
        return false;

    if (length > UINT32_CHAR_BUFFER_LENGTH)
        return false;

    



    RangedPtr<const CharT> cp(s, length + 1);
    const RangedPtr<const CharT> end(s + length, s, length + 1);

    uint32_t index = JS7_UNDEC(*cp++);
    uint32_t oldIndex = 0;
    uint32_t c = 0;

    if (index != 0) {
        while (JS7_ISDEC(*cp)) {
            oldIndex = index;
            c = JS7_UNDEC(*cp);
            index = 10 * index + c;
            cp++;
        }
    }

    
    if (cp != end)
        return false;

    



    if (oldIndex < UINT32_MAX / 10 || (oldIndex == UINT32_MAX / 10 && c <= (UINT32_MAX % 10))) {
        *indexp = index;
        return true;
    }

    return false;
}

template bool
JSFlatString::isIndexSlow(const Latin1Char *s, size_t length, uint32_t *indexp);

template bool
JSFlatString::isIndexSlow(const char16_t *s, size_t length, uint32_t *indexp);

bool
ScopedThreadSafeStringInspector::ensureChars(ThreadSafeContext *cx, const AutoCheckCannotGC &nogc)
{
    if (state_ != Uninitialized)
        return true;

    if (cx->isExclusiveContext()) {
        JSLinearString *linear = str_->ensureLinear(cx->asExclusiveContext());
        if (!linear)
            return false;
        if (linear->hasTwoByteChars()) {
            state_ = TwoByte;
            twoByteChars_ = linear->twoByteChars(nogc);
        } else {
            state_ = Latin1;
            latin1Chars_ = linear->latin1Chars(nogc);
        }
    } else {
        if (str_->isLinear()) {
            if (str_->hasLatin1Chars()) {
                state_ = Latin1;
                latin1Chars_ = str_->asLinear().latin1Chars(nogc);
            } else {
                state_ = TwoByte;
                twoByteChars_ = str_->asLinear().twoByteChars(nogc);
            }
        } else {
            if (str_->hasLatin1Chars()) {
                ScopedJSFreePtr<Latin1Char> chars;
                if (!str_->asRope().copyLatin1Chars(cx, chars))
                    return false;
                state_ = Latin1;
                latin1Chars_ = chars;
                scopedChars_ = chars.forget();
            } else {
                ScopedJSFreePtr<char16_t> chars;
                if (!str_->asRope().copyTwoByteChars(cx, chars))
                    return false;
                state_ = TwoByte;
                twoByteChars_ = chars;
                scopedChars_ = chars.forget();
            }
        }
    }

    MOZ_ASSERT(state_ != Uninitialized);
    return true;
}








#define R2(n) R(n),  R((n) + (1 << 0)),  R((n) + (2 << 0)),  R((n) + (3 << 0))
#define R4(n) R2(n), R2((n) + (1 << 2)), R2((n) + (2 << 2)), R2((n) + (3 << 2))
#define R6(n) R4(n), R4((n) + (1 << 4)), R4((n) + (2 << 4)), R4((n) + (3 << 4))
#define R7(n) R6(n), R6((n) + (1 << 6))





#define FROM_SMALL_CHAR(c) Latin1Char((c) + ((c) < 10 ? '0' :      \
                                             (c) < 36 ? 'a' - 10 : \
                                             'A' - 36))






#define TO_SMALL_CHAR(c) ((c) >= '0' && (c) <= '9' ? (c) - '0' :              \
                          (c) >= 'a' && (c) <= 'z' ? (c) - 'a' + 10 :         \
                          (c) >= 'A' && (c) <= 'Z' ? (c) - 'A' + 36 :         \
                          StaticStrings::INVALID_SMALL_CHAR)

#define R TO_SMALL_CHAR
const StaticStrings::SmallChar StaticStrings::toSmallChar[] = { R7(0) };
#undef R

bool
StaticStrings::init(JSContext *cx)
{
    AutoLockForExclusiveAccess lock(cx);
    AutoCompartment ac(cx, cx->runtime()->atomsCompartment());

    static_assert(UNIT_STATIC_LIMIT - 1 <= JSString::MAX_LATIN1_CHAR,
                  "Unit strings must fit in Latin1Char.");

    for (uint32_t i = 0; i < UNIT_STATIC_LIMIT; i++) {
        Latin1Char buffer[] = { Latin1Char(i), '\0' };
        JSFlatString *s = NewStringCopyN<NoGC>(cx, buffer, 1);
        if (!s)
            return false;
        unitStaticTable[i] = s->morphAtomizedStringIntoPermanentAtom();
    }

    for (uint32_t i = 0; i < NUM_SMALL_CHARS * NUM_SMALL_CHARS; i++) {
        Latin1Char buffer[] = { FROM_SMALL_CHAR(i >> 6), FROM_SMALL_CHAR(i & 0x3F), '\0' };
        JSFlatString *s = NewStringCopyN<NoGC>(cx, buffer, 2);
        if (!s)
            return false;
        length2StaticTable[i] = s->morphAtomizedStringIntoPermanentAtom();
    }

    for (uint32_t i = 0; i < INT_STATIC_LIMIT; i++) {
        if (i < 10) {
            intStaticTable[i] = unitStaticTable[i + '0'];
        } else if (i < 100) {
            size_t index = ((size_t)TO_SMALL_CHAR((i / 10) + '0') << 6) +
                TO_SMALL_CHAR((i % 10) + '0');
            intStaticTable[i] = length2StaticTable[index];
        } else {
            Latin1Char buffer[] = { Latin1Char('0' + (i / 100)),
                                    Latin1Char('0' + ((i / 10) % 10)),
                                    Latin1Char('0' + (i % 10)),
                                    '\0' };
            JSFlatString *s = NewStringCopyN<NoGC>(cx, buffer, 3);
            if (!s)
                return false;
            intStaticTable[i] = s->morphAtomizedStringIntoPermanentAtom();
        }
    }

    return true;
}

void
StaticStrings::trace(JSTracer *trc)
{
    

    for (uint32_t i = 0; i < UNIT_STATIC_LIMIT; i++)
        MarkPermanentAtom(trc, unitStaticTable[i], "unit-static-string");

    for (uint32_t i = 0; i < NUM_SMALL_CHARS * NUM_SMALL_CHARS; i++)
        MarkPermanentAtom(trc, length2StaticTable[i], "length2-static-string");

    
    for (uint32_t i = 0; i < INT_STATIC_LIMIT; i++)
        MarkPermanentAtom(trc, intStaticTable[i], "int-static-string");
}

template <typename CharT>
 bool
StaticStrings::isStatic(const CharT *chars, size_t length)
{
    switch (length) {
      case 1:
        return chars[0] < UNIT_STATIC_LIMIT;
      case 2:
        return fitsInSmallChar(chars[0]) && fitsInSmallChar(chars[1]);
      case 3:
        if ('1' <= chars[0] && chars[0] <= '9' &&
            '0' <= chars[1] && chars[1] <= '9' &&
            '0' <= chars[2] && chars[2] <= '9') {
            int i = (chars[0] - '0') * 100 +
                      (chars[1] - '0') * 10 +
                      (chars[2] - '0');

            return unsigned(i) < INT_STATIC_LIMIT;
        }
        return false;
      default:
        return false;
    }
}

 bool
StaticStrings::isStatic(JSAtom *atom)
{
    AutoCheckCannotGC nogc;
    return atom->hasLatin1Chars()
           ? isStatic(atom->latin1Chars(nogc), atom->length())
           : isStatic(atom->twoByteChars(nogc), atom->length());
}

AutoStableStringChars::~AutoStableStringChars()
{
    if (ownsChars_) {
        MOZ_ASSERT(state_ == Latin1 || state_ == TwoByte);
        if (state_ == Latin1)
            js_free(const_cast<Latin1Char*>(latin1Chars_));
        else
            js_free(const_cast<char16_t*>(twoByteChars_));
    }
}

bool
AutoStableStringChars::init(JSContext *cx, JSString *s)
{
    RootedLinearString linearString(cx, s->ensureLinear(cx));
    if (!linearString)
        return false;

    MOZ_ASSERT(state_ == Uninitialized);

    if (linearString->hasLatin1Chars()) {
        state_ = Latin1;
        latin1Chars_ = linearString->rawLatin1Chars();
    } else {
        state_ = TwoByte;
        twoByteChars_ = linearString->rawTwoByteChars();
    }

    s_ = linearString;
    return true;
}

bool
AutoStableStringChars::initTwoByte(JSContext *cx, JSString *s)
{
    RootedLinearString linearString(cx, s->ensureLinear(cx));
    if (!linearString)
        return false;

    MOZ_ASSERT(state_ == Uninitialized);

    if (linearString->hasTwoByteChars()) {
        state_ = TwoByte;
        twoByteChars_ = linearString->rawTwoByteChars();
        s_ = linearString;
        return true;
    }

    char16_t *chars = cx->pod_malloc<char16_t>(linearString->length() + 1);
    if (!chars)
        return false;

    CopyAndInflateChars(chars, linearString->rawLatin1Chars(),
                        linearString->length());
    chars[linearString->length()] = 0;

    state_ = TwoByte;
    ownsChars_ = true;
    twoByteChars_ = chars;
    s_ = linearString;
    return true;
}

#ifdef DEBUG
void
JSAtom::dump()
{
    fprintf(stderr, "JSAtom* (%p) = ", (void *) this);
    this->JSString::dump();
}
#endif 

JSLinearString *
js::NewDependentString(JSContext *cx, JSString *baseArg, size_t start, size_t length)
{
    if (length == 0)
        return cx->emptyString();

    JSLinearString *base = baseArg->ensureLinear(cx);
    if (!base)
        return nullptr;

    if (start == 0 && length == base->length())
        return base;

    if (base->hasTwoByteChars()) {
        AutoCheckCannotGC nogc;
        const char16_t *chars = base->twoByteChars(nogc) + start;
        if (JSLinearString *staticStr = cx->staticStrings().lookup(chars, length))
            return staticStr;
    } else {
        AutoCheckCannotGC nogc;
        const Latin1Char *chars = base->latin1Chars(nogc) + start;
        if (JSLinearString *staticStr = cx->staticStrings().lookup(chars, length))
            return staticStr;
    }

    return JSDependentString::new_(cx, base, start, length);
}

static bool
CanStoreCharsAsLatin1(const char16_t *s, size_t length)
{
    for (const char16_t *end = s + length; s < end; ++s) {
        if (*s > JSString::MAX_LATIN1_CHAR)
            return false;
    }

    return true;
}

static bool
CanStoreCharsAsLatin1(const Latin1Char *s, size_t length)
{
    MOZ_CRASH("Shouldn't be called for Latin1 chars");
}

template <AllowGC allowGC>
static MOZ_ALWAYS_INLINE JSInlineString *
NewFatInlineStringDeflated(ThreadSafeContext *cx, mozilla::Range<const char16_t> chars)
{
    size_t len = chars.length();
    Latin1Char *storage;
    JSInlineString *str = AllocateFatInlineString<allowGC>(cx, len, &storage);
    if (!str)
        return nullptr;

    for (size_t i = 0; i < len; i++) {
        MOZ_ASSERT(chars[i] <= JSString::MAX_LATIN1_CHAR);
        storage[i] = Latin1Char(chars[i]);
    }
    storage[len] = '\0';
    return str;
}

template <AllowGC allowGC>
static JSFlatString *
NewStringDeflated(ThreadSafeContext *cx, const char16_t *s, size_t n)
{
    if (JSFatInlineString::latin1LengthFits(n))
        return NewFatInlineStringDeflated<allowGC>(cx, mozilla::Range<const char16_t>(s, n));

    ScopedJSFreePtr<Latin1Char> news(cx->pod_malloc<Latin1Char>(n + 1));
    if (!news)
        return nullptr;

    for (size_t i = 0; i < n; i++) {
        MOZ_ASSERT(s[i] <= JSString::MAX_LATIN1_CHAR);
        news.get()[i] = Latin1Char(s[i]);
    }
    news[n] = '\0';

    JSFlatString *str = JSFlatString::new_<allowGC>(cx, news.get(), n);
    if (!str)
        return nullptr;

    news.forget();
    return str;
}

template <AllowGC allowGC>
static JSFlatString *
NewStringDeflated(ThreadSafeContext *cx, const Latin1Char *s, size_t n)
{
    MOZ_CRASH("Shouldn't be called for Latin1 chars");
}

template <AllowGC allowGC, typename CharT>
JSFlatString *
js::NewStringDontDeflate(ThreadSafeContext *cx, CharT *chars, size_t length)
{
    if (length == 1) {
        char16_t c = chars[0];
        if (StaticStrings::hasUnit(c)) {
            
            
            js_free(chars);
            return cx->staticStrings().getUnit(c);
        }
    }

    if (JSFatInlineString::lengthFits<CharT>(length)) {
        JSInlineString *str =
            NewFatInlineString<allowGC>(cx, mozilla::Range<const CharT>(chars, length));
        if (!str)
            return nullptr;

        js_free(chars);
        return str;
    }

    return JSFlatString::new_<allowGC>(cx, chars, length);
}

template JSFlatString *
js::NewStringDontDeflate<CanGC>(ThreadSafeContext *cx, char16_t *chars, size_t length);

template JSFlatString *
js::NewStringDontDeflate<NoGC>(ThreadSafeContext *cx, char16_t *chars, size_t length);

template JSFlatString *
js::NewStringDontDeflate<CanGC>(ThreadSafeContext *cx, Latin1Char *chars, size_t length);

template JSFlatString *
js::NewStringDontDeflate<NoGC>(ThreadSafeContext *cx, Latin1Char *chars, size_t length);

template <AllowGC allowGC, typename CharT>
JSFlatString *
js::NewString(ThreadSafeContext *cx, CharT *chars, size_t length)
{
    if (IsSame<CharT, char16_t>::value && CanStoreCharsAsLatin1(chars, length)) {
        if (length == 1) {
            char16_t c = chars[0];
            if (StaticStrings::hasUnit(c)) {
                js_free(chars);
                return cx->staticStrings().getUnit(c);
            }
        }

        JSFlatString *s = NewStringDeflated<allowGC>(cx, chars, length);
        if (!s)
            return nullptr;

        
        js_free(chars);
        return s;
    }

    return NewStringDontDeflate<allowGC>(cx, chars, length);
}

template JSFlatString *
js::NewString<CanGC>(ThreadSafeContext *cx, char16_t *chars, size_t length);

template JSFlatString *
js::NewString<NoGC>(ThreadSafeContext *cx, char16_t *chars, size_t length);

template JSFlatString *
js::NewString<CanGC>(ThreadSafeContext *cx, Latin1Char *chars, size_t length);

template JSFlatString *
js::NewString<NoGC>(ThreadSafeContext *cx, Latin1Char *chars, size_t length);

namespace js {

template <AllowGC allowGC, typename CharT>
JSFlatString *
NewStringCopyNDontDeflate(ThreadSafeContext *cx, const CharT *s, size_t n)
{
    if (JSFatInlineString::lengthFits<CharT>(n))
        return NewFatInlineString<allowGC>(cx, mozilla::Range<const CharT>(s, n));

    ScopedJSFreePtr<CharT> news(cx->pod_malloc<CharT>(n + 1));
    if (!news)
        return nullptr;

    PodCopy(news.get(), s, n);
    news[n] = 0;

    JSFlatString *str = JSFlatString::new_<allowGC>(cx, news.get(), n);
    if (!str)
        return nullptr;

    news.forget();
    return str;
}

template JSFlatString *
NewStringCopyNDontDeflate<CanGC>(ThreadSafeContext *cx, const char16_t *s, size_t n);

template JSFlatString *
NewStringCopyNDontDeflate<NoGC>(ThreadSafeContext *cx, const char16_t *s, size_t n);

template JSFlatString *
NewStringCopyNDontDeflate<CanGC>(ThreadSafeContext *cx, const Latin1Char *s, size_t n);

template JSFlatString *
NewStringCopyNDontDeflate<NoGC>(ThreadSafeContext *cx, const Latin1Char *s, size_t n);

template <AllowGC allowGC, typename CharT>
JSFlatString *
NewStringCopyN(ThreadSafeContext *cx, const CharT *s, size_t n)
{
    if (IsSame<CharT, char16_t>::value && CanStoreCharsAsLatin1(s, n))
        return NewStringDeflated<allowGC>(cx, s, n);

    return NewStringCopyNDontDeflate<allowGC>(cx, s, n);
}

template JSFlatString *
NewStringCopyN<CanGC>(ThreadSafeContext *cx, const char16_t *s, size_t n);

template JSFlatString *
NewStringCopyN<NoGC>(ThreadSafeContext *cx, const char16_t *s, size_t n);

template JSFlatString *
NewStringCopyN<CanGC>(ThreadSafeContext *cx, const Latin1Char *s, size_t n);

template JSFlatString *
NewStringCopyN<NoGC>(ThreadSafeContext *cx, const Latin1Char *s, size_t n);

} 
