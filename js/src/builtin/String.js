






var collatorCache = new Record();


function String_repeat(count) {
    
    CheckObjectCoercible(this);
    var S = ToString(this);

    
    var n = ToInteger(count);

    
    if (n < 0)
        ThrowError(JSMSG_NEGATIVE_REPETITION_COUNT); 

    if (!(n * S.length < (1 << 28)))
        ThrowError(JSMSG_RESULTING_STRING_TOO_LARGE); 

    
    n = n & ((1 << 28) - 1);

    
    var T = "";
    for (;;) {
        if (n & 1)
            T += S;
        n >>= 1;
        if (n)
            S += S;
        else
            break;
    }
    return T;
}

#define STRING_ITERATOR_SLOT_ITERATED_OBJECT 0
#define STRING_ITERATOR_SLOT_NEXT_INDEX 1


function String_iterator() {
    CheckObjectCoercible(this);
    var S = ToString(this);
    var iterator = NewStringIterator();
    UnsafeSetReservedSlot(iterator, STRING_ITERATOR_SLOT_ITERATED_OBJECT, S);
    UnsafeSetReservedSlot(iterator, STRING_ITERATOR_SLOT_NEXT_INDEX, 0);
    return iterator;
}

function StringIteratorIdentity() {
    return this;
}

function StringIteratorNext() {
    
    if (!IsObject(this) || !IsStringIterator(this))
        ThrowError(JSMSG_INCOMPATIBLE_METHOD, "StringIterator", "next", ToString(this));

    var S = UnsafeGetReservedSlot(this, STRING_ITERATOR_SLOT_ITERATED_OBJECT);
    var index = UnsafeGetReservedSlot(this, STRING_ITERATOR_SLOT_NEXT_INDEX);
    var size = S.length;

    if (index >= size) {
        return { value: undefined, done: true };
    }

    var charCount = 1;
    var first = callFunction(std_String_charCodeAt, S, index);
    if (first >= 0xD800 && first <= 0xDBFF && index + 1 < size) {
        var second = callFunction(std_String_charCodeAt, S, index + 1);
        if (second >= 0xDC00 && second <= 0xDFFF) {
            charCount = 2;
        }
    }

    UnsafeSetReservedSlot(this, STRING_ITERATOR_SLOT_NEXT_INDEX, index + charCount);
    var value = callFunction(std_String_substring, S, index, index + charCount);

    return { value: value, done: false };
}







function String_localeCompare(that) {
    
    CheckObjectCoercible(this);
    var S = ToString(this);
    var That = ToString(that);

    
    var locales = arguments.length > 1 ? arguments[1] : undefined;
    var options = arguments.length > 2 ? arguments[2] : undefined;

    
    var collator;
    if (locales === undefined && options === undefined) {
        
        
        if (collatorCache.collator === undefined)
            collatorCache.collator = intl_Collator(locales, options);
        collator = collatorCache.collator;
    } else {
        collator = intl_Collator(locales, options);
    }

    
    return intl_CompareStrings(collator, S, That);
}








function String_static_localeCompare(str1, str2) {
    if (arguments.length < 1)
        ThrowError(JSMSG_MISSING_FUN_ARG, 0, "String.localeCompare");
    var locales = arguments.length > 2 ? arguments[2] : undefined;
    var options = arguments.length > 3 ? arguments[3] : undefined;
    return callFunction(String_localeCompare, str1, str2, locales, options);
}
