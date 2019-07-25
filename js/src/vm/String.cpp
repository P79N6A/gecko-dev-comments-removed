







































#include "mozilla/RangedPtr.h"

#include "jsgcmark.h"

#include "String.h"
#include "String-inl.h"

using namespace mozilla;
using namespace js;

bool
JSString::isShort() const
{
    bool is_short = (getAllocKind() == gc::FINALIZE_SHORT_STRING);
    JS_ASSERT_IF(is_short, isFlat());
    return is_short;
}

bool
JSString::isFixed() const
{
    return isFlat() && !isExtensible();
}

bool
JSString::isInline() const
{
    return isFixed() && (d.u1.chars == d.inlineStorage || isShort());
}

bool
JSString::isExternal() const
{
    bool is_external = (getAllocKind() == gc::FINALIZE_EXTERNAL_STRING);
    JS_ASSERT_IF(is_external, isFixed());
    return is_external;
}

void
JSLinearString::mark(JSTracer *)
{
    JSLinearString *str = this;
    while (str->markIfUnmarked() && str->isDependent())
        str = str->asDependent().base();
}

size_t
JSString::charsHeapSize(JSUsableSizeFun usf)
{
    
    if (isRope())
        return 0;

    JS_ASSERT(isLinear());

    
    if (isDependent())
        return 0;

    JS_ASSERT(isFlat());

    
    if (isExtensible()) {
        JSExtensibleString &extensible = asExtensible();
        size_t usable = usf((void *)extensible.chars());
        return usable ? usable : asExtensible().capacity() * sizeof(jschar);
    }

    JS_ASSERT(isFixed());

    
    if (isExternal())
        return 0;

    
    if (isInline())
        return 0;

    
    JSFixedString &fixed = asFixed();
    size_t usable = usf((void *)fixed.chars());
    return usable ? usable : length() * sizeof(jschar);
}

static JS_ALWAYS_INLINE bool
AllocChars(JSContext *maybecx, size_t length, jschar **chars, size_t *capacity)
{
    




    size_t numChars = length + 1;

    




    static const size_t DOUBLING_MAX = 1024 * 1024;
    numChars = numChars > DOUBLING_MAX ? numChars + (numChars / 8) : RoundUpPow2(numChars);

    
    *capacity = numChars - 1;

    JS_STATIC_ASSERT(JSString::MAX_LENGTH * sizeof(jschar) < UINT32_MAX);
    size_t bytes = numChars * sizeof(jschar);
    *chars = (jschar *)(maybecx ? maybecx->malloc_(bytes) : OffTheBooks::malloc_(bytes));
    return *chars != NULL;
}

JSFlatString *
JSRope::flatten(JSContext *maybecx)
{
    































    const size_t wholeLength = length();
    size_t wholeCapacity;
    jschar *wholeChars;
    JSString *str = this;
    jschar *pos;

    if (this->leftChild()->isExtensible()) {
        JSExtensibleString &left = this->leftChild()->asExtensible();
        size_t capacity = left.capacity();
        if (capacity >= wholeLength) {
            wholeCapacity = capacity;
            wholeChars = const_cast<jschar *>(left.chars());
            size_t bits = left.d.lengthAndFlags;
            pos = wholeChars + (bits >> LENGTH_SHIFT);
            left.d.lengthAndFlags = bits ^ (EXTENSIBLE_FLAGS | DEPENDENT_BIT);
            left.d.s.u2.base = (JSLinearString *)this;  
            goto visit_right_child;
        }
    }

    if (!AllocChars(maybecx, wholeLength, &wholeChars, &wholeCapacity))
        return NULL;

    pos = wholeChars;
    first_visit_node: {
        JSString &left = *str->d.u1.left;
        str->d.u1.chars = pos;
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
            return &this->asFlat();
        }
        size_t progress = str->d.lengthAndFlags;
        str->d.lengthAndFlags = buildLengthAndFlags(pos - str->d.u1.chars, DEPENDENT_BIT);
        str->d.s.u2.base = (JSLinearString *)this;       
        str = str->d.s.u3.parent;
        if (progress == 0x200)
            goto visit_right_child;
        JS_ASSERT(progress == 0x300);
        goto finish_node;
    }
}

JSString * JS_FASTCALL
js_ConcatStrings(JSContext *cx, JSString *left, JSString *right)
{
    JS_ASSERT_IF(!left->isAtom(), left->compartment() == cx->compartment);
    JS_ASSERT_IF(!right->isAtom(), right->compartment() == cx->compartment);

    size_t leftLen = left->length();
    if (leftLen == 0)
        return right;

    size_t rightLen = right->length();
    if (rightLen == 0)
        return left;

    size_t wholeLength = leftLen + rightLen;
    if (!JSString::validateLength(cx, wholeLength))
        return NULL;

    if (JSShortString::lengthFits(wholeLength)) {
        JSShortString *str = js_NewGCShortString(cx);
        if (!str)
            return NULL;
        const jschar *leftChars = left->getChars(cx);
        if (!leftChars)
            return NULL;
        const jschar *rightChars = right->getChars(cx);
        if (!rightChars)
            return NULL;

        jschar *buf = str->init(wholeLength);
        PodCopy(buf, leftChars, leftLen);
        PodCopy(buf + leftLen, rightChars, rightLen);
        buf[wholeLength] = 0;
        return str;
    }

    return JSRope::new_(cx, left, right, wholeLength);
}

JSFixedString *
JSDependentString::undepend(JSContext *cx)
{
    JS_ASSERT(JSString::isDependent());

    size_t n = length();
    size_t size = (n + 1) * sizeof(jschar);
    jschar *s = (jschar *) cx->malloc_(size);
    if (!s)
        return NULL;

    PodCopy(s, chars(), n);
    s[n] = 0;

    d.lengthAndFlags = buildLengthAndFlags(n, FIXED_FLAGS);
    d.u1.chars = s;

    return &this->asFixed();
}

JSStringFinalizeOp JSExternalString::str_finalizers[JSExternalString::TYPE_LIMIT] = {
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL
};

bool
JSFlatString::isIndex(uint32 *indexp) const
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

    uint32 index = JS7_UNDEC(*cp++);
    uint32 oldIndex = 0;
    uint32 c = 0;

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








#define R2(n) R(n),  R((n) + (1 << 0)),  R((n) + (2 << 0)),  R((n) + (3 << 0))
#define R4(n) R2(n), R2((n) + (1 << 2)), R2((n) + (2 << 2)), R2((n) + (3 << 2))
#define R6(n) R4(n), R4((n) + (1 << 4)), R4((n) + (2 << 4)), R4((n) + (3 << 4))
#define R7(n) R6(n), R6((n) + (1 << 6))





#define FROM_SMALL_CHAR(c) ((c) + ((c) < 10 ? '0' :      \
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
    SwitchToCompartment sc(cx, cx->runtime->atomsCompartment);

    for (uint32 i = 0; i < UNIT_STATIC_LIMIT; i++) {
        jschar buffer[] = { i, 0x00 };
        JSFixedString *s = js_NewStringCopyN(cx, buffer, 1);
        if (!s)
            return false;
        unitStaticTable[i] = s->morphAtomizedStringIntoAtom();
    }

    for (uint32 i = 0; i < NUM_SMALL_CHARS * NUM_SMALL_CHARS; i++) {
        jschar buffer[] = { FROM_SMALL_CHAR(i >> 6), FROM_SMALL_CHAR(i & 0x3F), 0x00 };
        JSFixedString *s = js_NewStringCopyN(cx, buffer, 2);
        if (!s)
            return false;
        length2StaticTable[i] = s->morphAtomizedStringIntoAtom();
    }

    for (uint32 i = 0; i < INT_STATIC_LIMIT; i++) {
        if (i < 10) {
            intStaticTable[i] = unitStaticTable[i + '0'];
        } else if (i < 100) {
            size_t index = ((size_t)TO_SMALL_CHAR((i / 10) + '0') << 6) +
                TO_SMALL_CHAR((i % 10) + '0');
            intStaticTable[i] = length2StaticTable[index];
        } else {
            jschar buffer[] = { (i / 100) + '0', ((i / 10) % 10) + '0', (i % 10) + '0', 0x00 };
            JSFixedString *s = js_NewStringCopyN(cx, buffer, 3);
            if (!s)
                return false;
            intStaticTable[i] = s->morphAtomizedStringIntoAtom();
        }
    }

    initialized = true;
    return true;
}

void
StaticStrings::trace(JSTracer *trc)
{
    if (!initialized)
        return;

    for (uint32 i = 0; i < UNIT_STATIC_LIMIT; i++)
        MarkString(trc, unitStaticTable[i], "unit-static-string");

    for (uint32 i = 0; i < NUM_SMALL_CHARS * NUM_SMALL_CHARS; i++)
        MarkString(trc, length2StaticTable[i], "length2-static-string");

    
    for (uint32 i = 0; i < INT_STATIC_LIMIT; i++)
        MarkString(trc, intStaticTable[i], "int-static-string");
}

bool
StaticStrings::isStatic(JSAtom *atom)
{
    const jschar *chars = atom->chars();
    switch (atom->length()) {
      case 1:
        return (chars[0] < UNIT_STATIC_LIMIT);
      case 2:
        return (fitsInSmallChar(chars[0]) && fitsInSmallChar(chars[1]));
      case 3:
        if ('1' <= chars[0] && chars[0] <= '9' &&
            '0' <= chars[1] && chars[1] <= '9' &&
            '0' <= chars[2] && chars[2] <= '9') {
            jsint i = (chars[0] - '0') * 100 +
                      (chars[1] - '0') * 10 +
                      (chars[2] - '0');

            return (jsuint(i) < INT_STATIC_LIMIT);
        }
        return false;
      default:
        return false;
    }
}
