






function String_codePointAt(pos) {
    
    CheckObjectCoercible(this);
    var S = ToString(this);

    
    var position = ToInteger(pos);

    
    var size = S.length;

    
    if (position < 0 || position >= size)
        return undefined;

    
    var first = callFunction(std_String_charCodeAt, S, position);
    if (first < 0xD800 || first > 0xDBFF || position + 1 === size)
        return first;

    
    var second = callFunction(std_String_charCodeAt, S, position + 1);
    if (second < 0xDC00 || second > 0xDFFF)
        return first;

    
    return (first - 0xD800) * 0x400 + (second - 0xDC00) + 0x10000;
}

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


function String_static_fromCodePoint() {
    
    
    var length = arguments.length;

    
    var elements = new List();

    
    for (var nextIndex = 0; nextIndex < length; nextIndex++) {
        
        var next = arguments[nextIndex];
        
        var nextCP = ToNumber(next);

        
        if (nextCP !== ToInteger(nextCP) || std_isNaN(nextCP))
            ThrowError(JSMSG_NOT_A_CODEPOINT, ToString(nextCP));

        
        if (nextCP < 0 || nextCP > 0x10FFFF)
            ThrowError(JSMSG_NOT_A_CODEPOINT, ToString(nextCP));

        
        
        if (nextCP <= 0xFFFF) {
            elements.push(nextCP);
            continue;
        }

        elements.push((((nextCP - 0x10000) / 0x400) | 0) + 0xD800);
        elements.push((nextCP - 0x10000) % 0x400 + 0xDC00);
    }

    
    return callFunction(std_Function_apply, std_String_fromCharCode, null, elements);
}


function String_static_raw(callSite, ...substitutions) {
    
    
    var numberOfSubstitutions = substitutions.length;

    
    var cooked = ToObject(callSite);

    
    var raw = ToObject(cooked.raw);

    
    var literalSegments = ToLength(raw.length);

    
    if (literalSegments <= 0)
        return "";

    
    var resultString = "";

    
    var nextIndex = 0;

    
    while (true) {
        
        var nextSeg = ToString(raw[nextIndex]);

        
        resultString = resultString + nextSeg;

        
        if (nextIndex + 1 === literalSegments)
            
            return resultString;

        
        var nextSub;
        if (nextIndex < numberOfSubstitutions)
            nextSub = ToString(substitutions[nextIndex]);
        else
            nextSub = "";

        
        resultString = resultString + nextSub;

        
        nextIndex++;
    }
}








function String_static_localeCompare(str1, str2) {
    if (arguments.length < 1)
        ThrowError(JSMSG_MISSING_FUN_ARG, 0, "String.localeCompare");
    var locales = arguments.length > 2 ? arguments[2] : undefined;
    var options = arguments.length > 3 ? arguments[3] : undefined;
    return callFunction(String_localeCompare, str1, str2, locales, options);
}


function String_big() {
    CheckObjectCoercible(this);
    return "<big>" + ToString(this) + "</big>";
}


function String_blink() {
    CheckObjectCoercible(this);
    return "<blink>" + ToString(this) + "</blink>";
}


function String_bold() {
    CheckObjectCoercible(this);
    return "<b>" + ToString(this) + "</b>";
}


function String_fixed() {
    CheckObjectCoercible(this);
    return "<tt>" + ToString(this) + "</tt>";
}


function String_italics() {
    CheckObjectCoercible(this);
    return "<i>" + ToString(this) + "</i>";
}


function String_small() {
    CheckObjectCoercible(this);
    return "<small>" + ToString(this) + "</small>";
}


function String_strike() {
    CheckObjectCoercible(this);
    return "<strike>" + ToString(this) + "</strike>";
}


function String_sub() {
    CheckObjectCoercible(this);
    return "<sub>" + ToString(this) + "</sub>";
}


function String_sup() {
    CheckObjectCoercible(this);
    return "<sup>" + ToString(this) + "</sup>";
}

function EscapeAttributeValue(v) {
    var inputStr = ToString(v);
    var inputLen = inputStr.length;
    var outputStr = "";
    var chunkStart = 0;
    for (var i = 0; i < inputLen; i++) {
        if (inputStr[i] === '"') {
            outputStr += callFunction(std_String_substring, inputStr, chunkStart, i) + '&quot;';
            chunkStart = i + 1;
        }
    }
    if (chunkStart === 0)
        return inputStr;
    if (chunkStart < inputLen)
        outputStr += callFunction(std_String_substring, inputStr, chunkStart);
    return outputStr;
}


function String_anchor(name) {
    CheckObjectCoercible(this);
    var S = ToString(this);
    return '<a name="' + EscapeAttributeValue(name) + '">' + S + "</a>";
}


function String_fontcolor(color) {
    CheckObjectCoercible(this);
    var S = ToString(this);
    return '<font color="' + EscapeAttributeValue(color) + '">' + S + "</font>";
}


function String_fontsize(size) {
    CheckObjectCoercible(this);
    var S = ToString(this);
    return '<font size="' + EscapeAttributeValue(size) + '">' + S + "</font>";
}


function String_link(url) {
    CheckObjectCoercible(this);
    var S = ToString(this);
    return '<a href="' + EscapeAttributeValue(url) + '">' + S + "</a>";
}
