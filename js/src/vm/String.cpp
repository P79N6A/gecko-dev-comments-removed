





#include "vm/String-inl.h"

#include "mozilla/MathAlgorithms.h"
#include "mozilla/MemoryReporting.h"
#include "mozilla/PodOperations.h"
#include "mozilla/RangedPtr.h"

#include "gc/Marking.h"

#include "jscntxtinlines.h"
#include "jscompartmentinlines.h"

using namespace js;

using mozilla::PodCopy;
using mozilla::RangedPtr;
using mozilla::RoundUpPow2;

bool
JSString::isFatInline() const
{
    
    
    
    bool is_FatInline = (getAllocKind() == gc::FINALIZE_FAT_INLINE_STRING) && isInline();
    JS_ASSERT_IF(is_FatInline, isFlat());
    return is_FatInline;
}

bool
JSString::isExternal() const
{
    bool is_external = (getAllocKind() == gc::FINALIZE_EXTERNAL_STRING);
    JS_ASSERT_IF(is_external, isFlat());
    return is_external;
}

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
        return mallocSizeOf(extensible.chars());
    }

    
    if (isExternal())
        return 0;

    
    if (isInline())
        return 0;

    
    
    
    JSFlatString &flat = asFlat();
    return mallocSizeOf(flat.chars());
}

#ifdef DEBUG

void
JSString::dumpChars(const jschar *s, size_t n)
{
    if (n == SIZE_MAX) {
        n = 0;
        while (s[n])
            n++;
    }

    fputc('"', stderr);
    for (size_t i = 0; i < n; i++) {
        if (s[i] == '\n')
            fprintf(stderr, "\\n");
        else if (s[i] == '\t')
            fprintf(stderr, "\\t");
        else if (s[i] >= 32 && s[i] < 127)
            fputc(s[i], stderr);
        else if (s[i] <= 255)
            fprintf(stderr, "\\x%02x", (unsigned int) s[i]);
        else
            fprintf(stderr, "\\u%04x", (unsigned int) s[i]);
    }
    fputc('"', stderr);
}

void
JSString::dump()
{
    if (const jschar *chars = getChars(nullptr)) {
        fprintf(stderr, "JSString* (%p) = jschar * (%p) = ",
                (void *) this, (void *) chars);
        dumpChars(chars, length());
    } else {
        fprintf(stderr, "(oom in JSString::dump)");
    }
    fputc('\n', stderr);
}

bool
JSString::equals(const char *s)
{
    const jschar *c = getChars(nullptr);
    if (!c) {
        fprintf(stderr, "OOM in JSString::equals!\n");
        return false;
    }
    while (*c && *s) {
        if (*c != *s)
            return false;
        c++;
        s++;
    }
    return *c == *s;
}
#endif 

static MOZ_ALWAYS_INLINE bool
AllocChars(ThreadSafeContext *maybecx, size_t length, jschar **chars, size_t *capacity)
{
    




    size_t numChars = length + 1;

    




    static const size_t DOUBLING_MAX = 1024 * 1024;
    numChars = numChars > DOUBLING_MAX ? numChars + (numChars / 8) : RoundUpPow2(numChars);

    
    *capacity = numChars - 1;

    JS_STATIC_ASSERT(JSString::MAX_LENGTH * sizeof(jschar) < UINT32_MAX);
    size_t bytes = numChars * sizeof(jschar);
    *chars = (jschar *)(maybecx ? maybecx->malloc_(bytes) : js_malloc(bytes));
    return *chars != nullptr;
}

bool
JSRope::copyNonPureChars(ThreadSafeContext *cx, ScopedJSFreePtr<jschar> &out) const
{
    return copyNonPureCharsInternal(cx, out, false);
}

bool
JSRope::copyNonPureCharsZ(ThreadSafeContext *cx, ScopedJSFreePtr<jschar> &out) const
{
    return copyNonPureCharsInternal(cx, out, true);
}

bool
JSRope::copyNonPureCharsInternal(ThreadSafeContext *cx, ScopedJSFreePtr<jschar> &out,
                                 bool nullTerminate) const
{
    




    size_t n = length();
    if (cx)
        out.reset(cx->pod_malloc<jschar>(n + 1));
    else
        out.reset(js_pod_malloc<jschar>(n + 1));

    if (!out)
        return false;

    Vector<const JSString *, 8, SystemAllocPolicy> nodeStack;
    const JSString *str = this;
    jschar *pos = out;
    while (true) {
        if (str->isRope()) {
            if (!nodeStack.append(str->asRope().rightChild()))
                return false;
            str = str->asRope().leftChild();
        } else {
            size_t len = str->length();
            PodCopy(pos, str->asLinear().chars(), len);
            pos += len;
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

template<JSRope::UsingBarrier b>
JSFlatString *
JSRope::flattenInternal(ExclusiveContext *maybecx)
{
    































    const size_t wholeLength = length();
    size_t wholeCapacity;
    jschar *wholeChars;
    JSString *str = this;
    jschar *pos;

    
    JSRope *leftMostRope = this;
    while (leftMostRope->leftChild()->isRope())
        leftMostRope = &leftMostRope->leftChild()->asRope();

    if (leftMostRope->leftChild()->isExtensible()) {
        JSExtensibleString &left = leftMostRope->leftChild()->asExtensible();
        size_t capacity = left.capacity();
        if (capacity >= wholeLength) {
            



            while (str != leftMostRope) {
                JS_ASSERT(str->isRope());
                if (b == WithIncrementalBarrier) {
                    JSString::writeBarrierPre(str->d.u1.left);
                    JSString::writeBarrierPre(str->d.s.u2.right);
                }
                JSString *child = str->d.u1.left;
                str->d.u1.chars = left.chars();
                child->d.s.u3.parent = str;
                child->d.lengthAndFlags = 0x200;
                str = child;
            }
            if (b == WithIncrementalBarrier) {
                JSString::writeBarrierPre(str->d.u1.left);
                JSString::writeBarrierPre(str->d.s.u2.right);
            }
            str->d.u1.chars = left.chars();
            wholeCapacity = capacity;
            wholeChars = const_cast<jschar *>(left.chars());
            size_t bits = left.d.lengthAndFlags;
            pos = wholeChars + (bits >> LENGTH_SHIFT);
            JS_STATIC_ASSERT(!(EXTENSIBLE_FLAGS & DEPENDENT_FLAGS));
            left.d.lengthAndFlags = bits ^ (EXTENSIBLE_FLAGS | DEPENDENT_FLAGS);
            left.d.s.u2.base = (JSLinearString *)this;  
            StringWriteBarrierPostRemove(maybecx, &left.d.u1.left);
            StringWriteBarrierPost(maybecx, (JSString **)&left.d.s.u2.base);
            goto visit_right_child;
        }
    }

    if (!AllocChars(maybecx, wholeLength, &wholeChars, &wholeCapacity))
        return nullptr;

    pos = wholeChars;
    first_visit_node: {
        if (b == WithIncrementalBarrier) {
            JSString::writeBarrierPre(str->d.u1.left);
            JSString::writeBarrierPre(str->d.s.u2.right);
        }

        JSString &left = *str->d.u1.left;
        str->d.u1.chars = pos;
        StringWriteBarrierPostRemove(maybecx, &str->d.u1.left);
        if (left.isRope()) {
            left.d.s.u3.parent = str;          
            left.d.lengthAndFlags = 0x200;     
            str = &left;
            goto first_visit_node;
        }
        size_t len = left.length();
        PodCopy(pos, left.d.u1.chars, len);
        pos += len;
    }
    visit_right_child: {
        JSString &right = *str->d.s.u2.right;
        if (right.isRope()) {
            right.d.s.u3.parent = str;         
            right.d.lengthAndFlags = 0x300;    
            str = &right;
            goto first_visit_node;
        }
        size_t len = right.length();
        PodCopy(pos, right.d.u1.chars, len);
        pos += len;
    }
    finish_node: {
        if (str == this) {
            JS_ASSERT(pos == wholeChars + wholeLength);
            *pos = '\0';
            str->d.lengthAndFlags = buildLengthAndFlags(wholeLength, EXTENSIBLE_FLAGS);
            str->d.u1.chars = wholeChars;
            str->d.s.u2.capacity = wholeCapacity;
            StringWriteBarrierPostRemove(maybecx, &str->d.u1.left);
            StringWriteBarrierPostRemove(maybecx, &str->d.s.u2.right);
            return &this->asFlat();
        }
        size_t progress = str->d.lengthAndFlags;
        str->d.lengthAndFlags = buildLengthAndFlags(pos - str->d.u1.chars, DEPENDENT_FLAGS);
        str->d.s.u2.base = (JSLinearString *)this;       
        StringWriteBarrierPost(maybecx, (JSString **)&str->d.s.u2.base);
        str = str->d.s.u3.parent;
        if (progress == 0x200)
            goto visit_right_child;
        JS_ASSERT(progress == 0x300);
        goto finish_node;
    }
}

JSFlatString *
JSRope::flatten(ExclusiveContext *maybecx)
{
#if JSGC_INCREMENTAL
    if (zone()->needsBarrier())
        return flattenInternal<WithIncrementalBarrier>(maybecx);
    else
        return flattenInternal<NoBarrier>(maybecx);
#else
    return flattenInternal<NoBarrier>(maybecx);
#endif
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

    if (JSFatInlineString::lengthFits(wholeLength) && cx->isJSContext()) {
        JSFatInlineString *str = js_NewGCFatInlineString<allowGC>(cx);
        if (!str)
            return nullptr;

        ScopedThreadSafeStringInspector leftInspector(left);
        ScopedThreadSafeStringInspector rightInspector(right);
        if (!leftInspector.ensureChars(cx) || !rightInspector.ensureChars(cx))
            return nullptr;

        jschar *buf = str->init(wholeLength);
        PodCopy(buf, leftInspector.chars(), leftLen);
        PodCopy(buf + leftLen, rightInspector.chars(), rightLen);

        buf[wholeLength] = 0;
        return str;
    }

    return JSRope::new_<allowGC>(cx, left, right, wholeLength);
}

template JSString *
js::ConcatStrings<CanGC>(ThreadSafeContext *cx, HandleString left, HandleString right);

template JSString *
js::ConcatStrings<NoGC>(ThreadSafeContext *cx, JSString *left, JSString *right);

bool
JSDependentString::copyNonPureCharsZ(ThreadSafeContext *cx, ScopedJSFreePtr<jschar> &out) const
{
    JS_ASSERT(JSString::isDependent());

    size_t n = length();
    jschar *s = cx->pod_malloc<jschar>(n + 1);
    if (!s)
        return false;

    PodCopy(s, chars(), n);
    s[n] = 0;

    out.reset(s);
    return true;
}

JSFlatString *
JSDependentString::undepend(ExclusiveContext *cx)
{
    JS_ASSERT(JSString::isDependent());

    




    JSString::writeBarrierPre(base());

    size_t n = length();
    size_t size = (n + 1) * sizeof(jschar);
    jschar *s = (jschar *) cx->malloc_(size);
    if (!s)
        return nullptr;

    PodCopy(s, chars(), n);
    s[n] = 0;
    d.u1.chars = s;

    



    d.lengthAndFlags = buildLengthAndFlags(n, UNDEPENDED_FLAGS);

    return &this->asFlat();
}

bool
JSFlatString::isIndexSlow(uint32_t *indexp) const
{
    const jschar *s = charsZ();
    jschar ch = *s;

    if (!JS7_ISDEC(ch))
        return false;

    size_t n = length();
    if (n > UINT32_CHAR_BUFFER_LENGTH)
        return false;

    



    RangedPtr<const jschar> cp(s, n + 1);
    const RangedPtr<const jschar> end(s + n, s, n + 1);

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

bool
ScopedThreadSafeStringInspector::ensureChars(ThreadSafeContext *cx)
{
    if (chars_)
        return true;

    if (cx->isExclusiveContext()) {
        JSLinearString *linear = str_->ensureLinear(cx->asExclusiveContext());
        if (!linear)
            return false;
        chars_ = linear->chars();
    } else {
        if (str_->hasPureChars()) {
            chars_ = str_->pureChars();
        } else {
            if (!str_->copyNonPureChars(cx, scopedChars_))
                return false;
            chars_ = scopedChars_;
        }
    }

    JS_ASSERT(chars_);
    return true;
}








#define R2(n) R(n),  R((n) + (1 << 0)),  R((n) + (2 << 0)),  R((n) + (3 << 0))
#define R4(n) R2(n), R2((n) + (1 << 2)), R2((n) + (2 << 2)), R2((n) + (3 << 2))
#define R6(n) R4(n), R4((n) + (1 << 4)), R4((n) + (2 << 4)), R4((n) + (3 << 4))
#define R7(n) R6(n), R6((n) + (1 << 6))





#define FROM_SMALL_CHAR(c) jschar((c) + ((c) < 10 ? '0' :      \
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

    for (uint32_t i = 0; i < UNIT_STATIC_LIMIT; i++) {
        jschar buffer[] = { jschar(i), '\0' };
        JSFlatString *s = js_NewStringCopyN<NoGC>(cx, buffer, 1);
        if (!s)
            return false;
        unitStaticTable[i] = s->morphAtomizedStringIntoPermanentAtom();
    }

    for (uint32_t i = 0; i < NUM_SMALL_CHARS * NUM_SMALL_CHARS; i++) {
        jschar buffer[] = { FROM_SMALL_CHAR(i >> 6), FROM_SMALL_CHAR(i & 0x3F), '\0' };
        JSFlatString *s = js_NewStringCopyN<NoGC>(cx, buffer, 2);
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
            jschar buffer[] = { jschar('0' + (i / 100)),
                                jschar('0' + ((i / 10) % 10)),
                                jschar('0' + (i % 10)),
                                '\0' };
            JSFlatString *s = js_NewStringCopyN<NoGC>(cx, buffer, 3);
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

bool
StaticStrings::isStatic(JSAtom *atom)
{
    const jschar *chars = atom->chars();
    switch (atom->length()) {
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

#ifdef DEBUG
void
JSAtom::dump()
{
    fprintf(stderr, "JSAtom* (%p) = ", (void *) this);
    this->JSString::dump();
}
#endif 
