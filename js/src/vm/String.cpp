





#include "vm/String-inl.h"

#include "mozilla/MathAlgorithms.h"
#include "mozilla/MemoryReporting.h"
#include "mozilla/PodOperations.h"
#include "mozilla/RangedPtr.h"
#include "mozilla/TypeTraits.h"

#include "gc/Marking.h"
#include "js/UbiNode.h"

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

    MOZ_ASSERT(isLinear());

    
    if (isDependent())
        return 0;

    MOZ_ASSERT(isFlat());

    
    if (isExtensible()) {
        JSExtensibleString& extensible = asExtensible();
        return extensible.hasLatin1Chars()
               ? mallocSizeOf(extensible.rawLatin1Chars())
               : mallocSizeOf(extensible.rawTwoByteChars());
    }

    
    if (isExternal())
        return 0;

    
    if (isInline())
        return 0;

    
    
    
    JSFlatString& flat = asFlat();
    return flat.hasLatin1Chars()
           ? mallocSizeOf(flat.rawLatin1Chars())
           : mallocSizeOf(flat.rawTwoByteChars());
}

size_t
JS::ubi::Concrete<JSString>::size(mozilla::MallocSizeOf mallocSizeOf) const
{
    JSString &str = get();
    size_t size = str.isFatInline() ? sizeof(JSFatInlineString) : sizeof(JSString);

    
    
    MOZ_ASSERT(!IsInsideNursery(&str));
    size += str.sizeOfExcludingThis(mallocSizeOf);

    return size;
}

template<> const char16_t JS::ubi::TracerConcrete<JSString>::concreteTypeName[] =
    MOZ_UTF16("JSString");

#ifdef DEBUG

template <typename CharT>
 void
JSString::dumpChars(const CharT* s, size_t n, FILE* fp)
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
JSString::dumpChars(const Latin1Char* s, size_t n, FILE* fp);

template void
JSString::dumpChars(const char16_t* s, size_t n, FILE* fp);

void
JSString::dumpCharsNoNewline(FILE* fp)
{
    if (JSLinearString* linear = ensureLinear(nullptr)) {
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
    if (JSLinearString* linear = ensureLinear(nullptr)) {
        AutoCheckCannotGC nogc;
        if (hasLatin1Chars()) {
            const Latin1Char* chars = linear->latin1Chars(nogc);
            fprintf(stderr, "JSString* (%p) = Latin1Char * (%p) = ", (void*) this,
                    (void*) chars);
            dumpChars(chars, length(), stderr);
        } else {
            const char16_t* chars = linear->twoByteChars(nogc);
            fprintf(stderr, "JSString* (%p) = char16_t * (%p) = ", (void*) this,
                    (void*) chars);
            dumpChars(chars, length(), stderr);
        }
    } else {
        fprintf(stderr, "(oom in JSString::dump)");
    }
    fputc('\n', stderr);
}

void
JSString::dumpRepresentation(FILE* fp, int indent) const
{
    if      (isRope())          asRope()        .dumpRepresentation(fp, indent);
    else if (isDependent())     asDependent()   .dumpRepresentation(fp, indent);
    else if (isExternal())      asExternal()    .dumpRepresentation(fp, indent);
    else if (isExtensible())    asExtensible()  .dumpRepresentation(fp, indent);
    else if (isInline())        asInline()      .dumpRepresentation(fp, indent);
    else if (isFlat())          asFlat()        .dumpRepresentation(fp, indent);
    else
        MOZ_CRASH("Unexpected JSString representation");
}

void
JSString::dumpRepresentationHeader(FILE* fp, int indent, const char* subclass) const
{
    uint32_t flags = d.u1.flags;
    
    
    fprintf(fp, "((%s*) %p) length: %zu  flags: 0x%x", subclass, this, length(), flags);
    if (flags & FLAT_BIT)               fputs(" FLAT", fp);
    if (flags & HAS_BASE_BIT)           fputs(" HAS_BASE", fp);
    if (flags & INLINE_CHARS_BIT)       fputs(" INLINE_CHARS", fp);
    if (flags & ATOM_BIT)               fputs(" ATOM", fp);
    if (isPermanentAtom())              fputs(" PERMANENT", fp);
    if (flags & LATIN1_CHARS_BIT)       fputs(" LATIN1", fp);
    fputc('\n', fp);
}

void
JSLinearString::dumpRepresentationChars(FILE* fp, int indent) const
{
    if (hasLatin1Chars()) {
        fprintf(fp, "%*schars: ((Latin1Char*) %p) ", indent, "", rawLatin1Chars());
        dumpChars(rawLatin1Chars(), length());
    } else {
        fprintf(fp, "%*schars: ((char16_t*) %p) ", indent, "", rawTwoByteChars());
        dumpChars(rawTwoByteChars(), length());
    }
    fputc('\n', fp);
}

bool
JSString::equals(const char* s)
{
    JSLinearString* linear = ensureLinear(nullptr);
    if (!linear) {
        fprintf(stderr, "OOM in JSString::equals!\n");
        return false;
    }

    return StringEqualsAscii(linear, s);
}
#endif 

template <typename CharT>
static MOZ_ALWAYS_INLINE bool
AllocChars(JSString* str, size_t length, CharT** chars, size_t* capacity)
{
    




    size_t numChars = length + 1;

    




    static const size_t DOUBLING_MAX = 1024 * 1024;
    numChars = numChars > DOUBLING_MAX ? numChars + (numChars / 8) : RoundUpPow2(numChars);

    
    *capacity = numChars - 1;

    JS_STATIC_ASSERT(JSString::MAX_LENGTH * sizeof(CharT) < UINT32_MAX);
    *chars = str->zone()->pod_malloc<CharT>(numChars);
    return *chars != nullptr;
}

bool
JSRope::copyLatin1CharsZ(ExclusiveContext* cx, ScopedJSFreePtr<Latin1Char>& out) const
{
    return copyCharsInternal<Latin1Char>(cx, out, true);
}

bool
JSRope::copyTwoByteCharsZ(ExclusiveContext* cx, ScopedJSFreePtr<char16_t>& out) const
{
    return copyCharsInternal<char16_t>(cx, out, true);
}

bool
JSRope::copyLatin1Chars(ExclusiveContext* cx, ScopedJSFreePtr<Latin1Char>& out) const
{
    return copyCharsInternal<Latin1Char>(cx, out, false);
}

bool
JSRope::copyTwoByteChars(ExclusiveContext* cx, ScopedJSFreePtr<char16_t>& out) const
{
    return copyCharsInternal<char16_t>(cx, out, false);
}

template <typename CharT>
bool
JSRope::copyCharsInternal(ExclusiveContext* cx, ScopedJSFreePtr<CharT>& out,
                          bool nullTerminate) const
{
    




    size_t n = length();
    if (cx)
        out.reset(cx->pod_malloc<CharT>(n + 1));
    else
        out.reset(js_pod_malloc<CharT>(n + 1));

    if (!out)
        return false;

    Vector<const JSString*, 8, SystemAllocPolicy> nodeStack;
    const JSString* str = this;
    CharT* pos = out;
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

    MOZ_ASSERT(pos == out + n);

    if (nullTerminate)
        out[n] = 0;

    return true;
}

#ifdef DEBUG
void
JSRope::dumpRepresentation(FILE* fp, int indent) const
{
    dumpRepresentationHeader(fp, indent, "JSRope");
    indent += 2;

    fprintf(fp, "%*sleft:  ", indent, "");
    leftChild()->dumpRepresentation(fp, indent);

    fprintf(fp, "%*sright: ", indent, "");
    rightChild()->dumpRepresentation(fp, indent);
}
#endif

namespace js {

template <>
void
CopyChars(char16_t* dest, const JSLinearString& str)
{
    AutoCheckCannotGC nogc;
    if (str.hasTwoByteChars())
        PodCopy(dest, str.twoByteChars(nogc), str.length());
    else
        CopyAndInflateChars(dest, str.latin1Chars(nogc), str.length());
}

template <>
void
CopyChars(Latin1Char* dest, const JSLinearString& str)
{
    AutoCheckCannotGC nogc;
    if (str.hasLatin1Chars()) {
        PodCopy(dest, str.latin1Chars(nogc), str.length());
    } else {
        







        size_t len = str.length();
        const char16_t* chars = str.twoByteChars(nogc);
        for (size_t i = 0; i < len; i++) {
            MOZ_ASSERT(chars[i] <= JSString::MAX_LATIN1_CHAR);
            dest[i] = chars[i];
        }
    }
}

} 

template<JSRope::UsingBarrier b, typename CharT>
JSFlatString*
JSRope::flattenInternal(ExclusiveContext* maybecx)
{
    




















































    const size_t wholeLength = length();
    size_t wholeCapacity;
    CharT* wholeChars;
    JSString* str = this;
    CharT* pos;

    



    static const uintptr_t Tag_Mask = 0x3;
    static const uintptr_t Tag_FinishNode = 0x0;
    static const uintptr_t Tag_VisitRightChild = 0x1;

    AutoCheckCannotGC nogc;

    
    JSRope* leftMostRope = this;
    while (leftMostRope->leftChild()->isRope())
        leftMostRope = &leftMostRope->leftChild()->asRope();

    if (leftMostRope->leftChild()->isExtensible()) {
        JSExtensibleString& left = leftMostRope->leftChild()->asExtensible();
        size_t capacity = left.capacity();
        if (capacity >= wholeLength && left.hasTwoByteChars() == IsSame<CharT, char16_t>::value) {
            



            MOZ_ASSERT(str->isRope());
            while (str != leftMostRope) {
                if (b == WithIncrementalBarrier) {
                    JSString::writeBarrierPre(str->d.s.u2.left);
                    JSString::writeBarrierPre(str->d.s.u3.right);
                }
                JSString* child = str->d.s.u2.left;
                MOZ_ASSERT(child->isRope());
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
            wholeChars = const_cast<CharT*>(left.nonInlineChars<CharT>(nogc));
            pos = wholeChars + left.d.u1.length;
            JS_STATIC_ASSERT(!(EXTENSIBLE_FLAGS & DEPENDENT_FLAGS));
            left.d.u1.flags ^= (EXTENSIBLE_FLAGS | DEPENDENT_FLAGS);
            left.d.s.u3.base = (JSLinearString*)this;  
            StringWriteBarrierPostRemove(maybecx, &left.d.s.u2.left);
            StringWriteBarrierPost(maybecx, (JSString**)&left.d.s.u3.base);
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

        JSString& left = *str->d.s.u2.left;
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
        JSString& right = *str->d.s.u3.right;
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
            MOZ_ASSERT(pos == wholeChars + wholeLength);
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
        str->d.s.u3.base = (JSLinearString*)this;       
        StringWriteBarrierPost(maybecx, (JSString**)&str->d.s.u3.base);
        str = (JSString*)(flattenData & ~Tag_Mask);
        if ((flattenData & Tag_Mask) == Tag_VisitRightChild)
            goto visit_right_child;
        MOZ_ASSERT((flattenData & Tag_Mask) == Tag_FinishNode);
        goto finish_node;
    }
}

template<JSRope::UsingBarrier b>
JSFlatString*
JSRope::flattenInternal(ExclusiveContext* maybecx)
{
    if (hasTwoByteChars())
        return flattenInternal<b, char16_t>(maybecx);
    return flattenInternal<b, Latin1Char>(maybecx);
}

JSFlatString*
JSRope::flatten(ExclusiveContext* maybecx)
{
    if (zone()->needsIncrementalBarrier())
        return flattenInternal<WithIncrementalBarrier>(maybecx);
    return flattenInternal<NoBarrier>(maybecx);
}

template <AllowGC allowGC>
JSString*
js::ConcatStrings(ExclusiveContext* cx,
                  typename MaybeRooted<JSString*, allowGC>::HandleType left,
                  typename MaybeRooted<JSString*, allowGC>::HandleType right)
{
    MOZ_ASSERT_IF(!left->isAtom(), cx->isInsideCurrentZone(left));
    MOZ_ASSERT_IF(!right->isAtom(), cx->isInsideCurrentZone(right));

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
    bool canUseInline = isLatin1
                        ? JSInlineString::lengthFits<Latin1Char>(wholeLength)
                        : JSInlineString::lengthFits<char16_t>(wholeLength);
    if (canUseInline && cx->isJSContext()) {
        Latin1Char* latin1Buf = nullptr;  
        char16_t* twoByteBuf = nullptr;  
        JSInlineString* str = isLatin1
            ? AllocateInlineString<allowGC>(cx, wholeLength, &latin1Buf)
            : AllocateInlineString<allowGC>(cx, wholeLength, &twoByteBuf);
        if (!str)
            return nullptr;

        AutoCheckCannotGC nogc;
        JSLinearString* leftLinear = left->ensureLinear(cx);
        if (!leftLinear)
            return nullptr;
        JSLinearString* rightLinear = right->ensureLinear(cx);
        if (!rightLinear)
            return nullptr;

        if (isLatin1) {
            PodCopy(latin1Buf, leftLinear->latin1Chars(nogc), leftLen);
            PodCopy(latin1Buf + leftLen, rightLinear->latin1Chars(nogc), rightLen);
            latin1Buf[wholeLength] = 0;
        } else {
            if (leftLinear->hasTwoByteChars())
                PodCopy(twoByteBuf, leftLinear->twoByteChars(nogc), leftLen);
            else
                CopyAndInflateChars(twoByteBuf, leftLinear->latin1Chars(nogc), leftLen);
            if (rightLinear->hasTwoByteChars())
                PodCopy(twoByteBuf + leftLen, rightLinear->twoByteChars(nogc), rightLen);
            else
                CopyAndInflateChars(twoByteBuf + leftLen, rightLinear->latin1Chars(nogc), rightLen);
            twoByteBuf[wholeLength] = 0;
        }

        return str;
    }

    return JSRope::new_<allowGC>(cx, left, right, wholeLength);
}

template JSString*
js::ConcatStrings<CanGC>(ExclusiveContext* cx, HandleString left, HandleString right);

template JSString*
js::ConcatStrings<NoGC>(ExclusiveContext* cx, JSString* left, JSString* right);

template <typename CharT>
JSFlatString*
JSDependentString::undependInternal(ExclusiveContext* cx)
{
    




    JSString::writeBarrierPre(base());

    size_t n = length();
    CharT* s = cx->pod_malloc<CharT>(n + 1);
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

JSFlatString*
JSDependentString::undepend(ExclusiveContext* cx)
{
    MOZ_ASSERT(JSString::isDependent());
    return hasLatin1Chars()
           ? undependInternal<Latin1Char>(cx)
           : undependInternal<char16_t>(cx);
}

#ifdef DEBUG
void
JSDependentString::dumpRepresentation(FILE* fp, int indent) const
{
    dumpRepresentationHeader(fp, indent, "JSDependentString");
    indent += 2;

    fprintf(fp, "%*soffset: %zu\n", indent, "", baseOffset());
    fprintf(fp, "%*sbase: ", indent, "");
    base()->dumpRepresentation(fp, indent);
}
#endif

template <typename CharT>
 bool
JSFlatString::isIndexSlow(const CharT* s, size_t length, uint32_t* indexp)
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
JSFlatString::isIndexSlow(const Latin1Char* s, size_t length, uint32_t* indexp);

template bool
JSFlatString::isIndexSlow(const char16_t* s, size_t length, uint32_t* indexp);








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

#undef R2
#undef R4
#undef R6
#undef R7

bool
StaticStrings::init(JSContext* cx)
{
    AutoLockForExclusiveAccess lock(cx);
    AutoCompartment ac(cx, cx->runtime()->atomsCompartment());

    static_assert(UNIT_STATIC_LIMIT - 1 <= JSString::MAX_LATIN1_CHAR,
                  "Unit strings must fit in Latin1Char.");

    for (uint32_t i = 0; i < UNIT_STATIC_LIMIT; i++) {
        Latin1Char buffer[] = { Latin1Char(i), '\0' };
        JSFlatString* s = NewStringCopyN<NoGC>(cx, buffer, 1);
        if (!s)
            return false;
        unitStaticTable[i] = s->morphAtomizedStringIntoPermanentAtom();
    }

    for (uint32_t i = 0; i < NUM_SMALL_CHARS * NUM_SMALL_CHARS; i++) {
        Latin1Char buffer[] = { FROM_SMALL_CHAR(i >> 6), FROM_SMALL_CHAR(i & 0x3F), '\0' };
        JSFlatString* s = NewStringCopyN<NoGC>(cx, buffer, 2);
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
            JSFlatString* s = NewStringCopyN<NoGC>(cx, buffer, 3);
            if (!s)
                return false;
            intStaticTable[i] = s->morphAtomizedStringIntoPermanentAtom();
        }
    }

    return true;
}

void
StaticStrings::trace(JSTracer* trc)
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
StaticStrings::isStatic(const CharT* chars, size_t length)
{
    switch (length) {
      case 1: {
        char16_t c = chars[0];
        return c < UNIT_STATIC_LIMIT;
      }
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
StaticStrings::isStatic(JSAtom* atom)
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
AutoStableStringChars::init(JSContext* cx, JSString* s)
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
AutoStableStringChars::initTwoByte(JSContext* cx, JSString* s)
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

    char16_t* chars = cx->pod_malloc<char16_t>(linearString->length() + 1);
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
    fprintf(stderr, "JSAtom* (%p) = ", (void*) this);
    this->JSString::dump();
}

void
JSExternalString::dumpRepresentation(FILE* fp, int indent) const
{
    dumpRepresentationHeader(fp, indent, "JSExternalString");
    indent += 2;

    fprintf(fp, "%*sfinalizer: ((JSStringFinalizer*) %p)\n", indent, "", externalFinalizer());
    fprintf(fp, "%*sbase: ", indent, "");
    base()->dumpRepresentation(fp, indent);
}
#endif 

JSLinearString*
js::NewDependentString(JSContext* cx, JSString* baseArg, size_t start, size_t length)
{
    if (length == 0)
        return cx->emptyString();

    JSLinearString* base = baseArg->ensureLinear(cx);
    if (!base)
        return nullptr;

    if (start == 0 && length == base->length())
        return base;

    if (base->hasTwoByteChars()) {
        AutoCheckCannotGC nogc;
        const char16_t* chars = base->twoByteChars(nogc) + start;
        if (JSLinearString* staticStr = cx->staticStrings().lookup(chars, length))
            return staticStr;
    } else {
        AutoCheckCannotGC nogc;
        const Latin1Char* chars = base->latin1Chars(nogc) + start;
        if (JSLinearString* staticStr = cx->staticStrings().lookup(chars, length))
            return staticStr;
    }

    return JSDependentString::new_(cx, base, start, length);
}

static bool
CanStoreCharsAsLatin1(const char16_t* s, size_t length)
{
    for (const char16_t* end = s + length; s < end; ++s) {
        if (*s > JSString::MAX_LATIN1_CHAR)
            return false;
    }

    return true;
}

static bool
CanStoreCharsAsLatin1(const Latin1Char* s, size_t length)
{
    MOZ_CRASH("Shouldn't be called for Latin1 chars");
}

template <AllowGC allowGC>
static MOZ_ALWAYS_INLINE JSInlineString*
NewInlineStringDeflated(ExclusiveContext* cx, mozilla::Range<const char16_t> chars)
{
    size_t len = chars.length();
    Latin1Char* storage;
    JSInlineString* str = AllocateInlineString<allowGC>(cx, len, &storage);
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
static JSFlatString*
NewStringDeflated(ExclusiveContext* cx, const char16_t* s, size_t n)
{
    if (JSInlineString::lengthFits<Latin1Char>(n))
        return NewInlineStringDeflated<allowGC>(cx, mozilla::Range<const char16_t>(s, n));

    ScopedJSFreePtr<Latin1Char> news(cx->pod_malloc<Latin1Char>(n + 1));
    if (!news)
        return nullptr;

    for (size_t i = 0; i < n; i++) {
        MOZ_ASSERT(s[i] <= JSString::MAX_LATIN1_CHAR);
        news.get()[i] = Latin1Char(s[i]);
    }
    news[n] = '\0';

    JSFlatString* str = JSFlatString::new_<allowGC>(cx, news.get(), n);
    if (!str)
        return nullptr;

    news.forget();
    return str;
}

template <AllowGC allowGC>
static JSFlatString*
NewStringDeflated(ExclusiveContext* cx, const Latin1Char* s, size_t n)
{
    MOZ_CRASH("Shouldn't be called for Latin1 chars");
}

template <AllowGC allowGC, typename CharT>
JSFlatString*
js::NewStringDontDeflate(ExclusiveContext* cx, CharT* chars, size_t length)
{
    if (length == 1) {
        char16_t c = chars[0];
        if (StaticStrings::hasUnit(c)) {
            
            
            js_free(chars);
            return cx->staticStrings().getUnit(c);
        }
    }

    if (JSInlineString::lengthFits<CharT>(length)) {
        JSInlineString* str =
            NewInlineString<allowGC>(cx, mozilla::Range<const CharT>(chars, length));
        if (!str)
            return nullptr;

        js_free(chars);
        return str;
    }

    return JSFlatString::new_<allowGC>(cx, chars, length);
}

template JSFlatString*
js::NewStringDontDeflate<CanGC>(ExclusiveContext* cx, char16_t* chars, size_t length);

template JSFlatString*
js::NewStringDontDeflate<NoGC>(ExclusiveContext* cx, char16_t* chars, size_t length);

template JSFlatString*
js::NewStringDontDeflate<CanGC>(ExclusiveContext* cx, Latin1Char* chars, size_t length);

template JSFlatString*
js::NewStringDontDeflate<NoGC>(ExclusiveContext* cx, Latin1Char* chars, size_t length);

template <AllowGC allowGC, typename CharT>
JSFlatString*
js::NewString(ExclusiveContext* cx, CharT* chars, size_t length)
{
    if (IsSame<CharT, char16_t>::value && CanStoreCharsAsLatin1(chars, length)) {
        if (length == 1) {
            char16_t c = chars[0];
            if (StaticStrings::hasUnit(c)) {
                js_free(chars);
                return cx->staticStrings().getUnit(c);
            }
        }

        JSFlatString* s = NewStringDeflated<allowGC>(cx, chars, length);
        if (!s)
            return nullptr;

        
        js_free(chars);
        return s;
    }

    return NewStringDontDeflate<allowGC>(cx, chars, length);
}

template JSFlatString*
js::NewString<CanGC>(ExclusiveContext* cx, char16_t* chars, size_t length);

template JSFlatString*
js::NewString<NoGC>(ExclusiveContext* cx, char16_t* chars, size_t length);

template JSFlatString*
js::NewString<CanGC>(ExclusiveContext* cx, Latin1Char* chars, size_t length);

template JSFlatString*
js::NewString<NoGC>(ExclusiveContext* cx, Latin1Char* chars, size_t length);

namespace js {

template <AllowGC allowGC, typename CharT>
JSFlatString*
NewStringCopyNDontDeflate(ExclusiveContext* cx, const CharT* s, size_t n)
{
    if (JSInlineString::lengthFits<CharT>(n))
        return NewInlineString<allowGC>(cx, mozilla::Range<const CharT>(s, n));

    ScopedJSFreePtr<CharT> news(cx->pod_malloc<CharT>(n + 1));
    if (!news)
        return nullptr;

    PodCopy(news.get(), s, n);
    news[n] = 0;

    JSFlatString* str = JSFlatString::new_<allowGC>(cx, news.get(), n);
    if (!str)
        return nullptr;

    news.forget();
    return str;
}

template JSFlatString*
NewStringCopyNDontDeflate<CanGC>(ExclusiveContext* cx, const char16_t* s, size_t n);

template JSFlatString*
NewStringCopyNDontDeflate<NoGC>(ExclusiveContext* cx, const char16_t* s, size_t n);

template JSFlatString*
NewStringCopyNDontDeflate<CanGC>(ExclusiveContext* cx, const Latin1Char* s, size_t n);

template JSFlatString*
NewStringCopyNDontDeflate<NoGC>(ExclusiveContext* cx, const Latin1Char* s, size_t n);

template <AllowGC allowGC, typename CharT>
JSFlatString*
NewStringCopyN(ExclusiveContext* cx, const CharT* s, size_t n)
{
    if (IsSame<CharT, char16_t>::value && CanStoreCharsAsLatin1(s, n))
        return NewStringDeflated<allowGC>(cx, s, n);

    return NewStringCopyNDontDeflate<allowGC>(cx, s, n);
}

template JSFlatString*
NewStringCopyN<CanGC>(ExclusiveContext* cx, const char16_t* s, size_t n);

template JSFlatString*
NewStringCopyN<NoGC>(ExclusiveContext* cx, const char16_t* s, size_t n);

template JSFlatString*
NewStringCopyN<CanGC>(ExclusiveContext* cx, const Latin1Char* s, size_t n);

template JSFlatString*
NewStringCopyN<NoGC>(ExclusiveContext* cx, const Latin1Char* s, size_t n);

} 

#ifdef DEBUG
void
JSExtensibleString::dumpRepresentation(FILE* fp, int indent) const
{
    dumpRepresentationHeader(fp, indent, "JSExtensibleString");
    indent += 2;

    fprintf(fp, "%*scapacity: %zu\n", indent, "", capacity());
    dumpRepresentationChars(fp, indent);
}

void
JSInlineString::dumpRepresentation(FILE* fp, int indent) const
{
    dumpRepresentationHeader(fp, indent,
                             isFatInline() ? "JSFatInlineString" : "JSThinInlineString");
    indent += 2;

    dumpRepresentationChars(fp, indent);
}

void
JSFlatString::dumpRepresentation(FILE* fp, int indent) const
{
    dumpRepresentationHeader(fp, indent, "JSFlatString");
    indent += 2;

    dumpRepresentationChars(fp, indent);
}
#endif
