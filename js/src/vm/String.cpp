







































#include "mozilla/RangedPtr.h"

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
    while (!str->isStaticAtom() && str->markIfUnmarked() && str->isDependent())
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

    
    if (isStaticAtom())
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

    if (wholeLength > JSString::MAX_LENGTH) {
        if (JS_ON_TRACE(cx)) {
            if (!CanLeaveTrace(cx))
                return NULL;
            LeaveTrace(cx);
        }
        js_ReportAllocationOverflow(cx);
        return NULL;
    }

    return JSRope::new_(cx, left, right, wholeLength);
}

JSFixedString *
JSDependentString::undepend(JSContext *cx)
{
    JS_ASSERT(isDependent());

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
JSFlatString::isElement(uint32 *indexp) const
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
