






function String_substring(start, end) {
    
    CheckObjectCoercible(this);
    var str = ToString(this);

    
    var len = str.length;

    
    var intStart = ToInteger(start);

    
    var intEnd = (end === undefined) ? len : ToInteger(end);

    
    var finalStart = std_Math_min(std_Math_max(intStart, 0), len);

    
    var finalEnd = std_Math_min(std_Math_max(intEnd, 0), len);

    
    var from, to;
    if (finalStart < finalEnd) {
        from = finalStart;
        to = finalEnd;
    } else {
        from = finalEnd;
        to = finalStart;
    }

    
    
    
    
    return SubstringKernel(str, from | 0, (to - from) | 0);
}

function String_static_substring(string, start, end) {
    if (arguments.length < 1)
        ThrowTypeError(JSMSG_MISSING_FUN_ARG, 0, 'String.substring');
    return callFunction(String_substring, string, start, end);
}


function String_substr(start, length) {
    
    CheckObjectCoercible(this);
    var str = ToString(this);

    
    var intStart = ToInteger(start);

    
    var size = str.length;
    
    
    var end = (length === undefined) ? size : ToInteger(length);

    
    if (intStart < 0)
        intStart = std_Math_max(intStart + size, 0);

    
    var resultLength = std_Math_min(std_Math_max(end, 0), size - intStart)

    
    if (resultLength <= 0)
        return "";

    
    
    
    
    return SubstringKernel(str, intStart | 0, resultLength | 0);
}

function String_static_substr(string, start, length) {
    if (arguments.length < 1)
        ThrowTypeError(JSMSG_MISSING_FUN_ARG, 0, 'String.substr');
    return callFunction(String_substr, string, start, length);
}


function String_slice(start, end) {
    
    CheckObjectCoercible(this);
    var str = ToString(this);

    
    var len = str.length;

    
    var intStart = ToInteger(start);

    
    var intEnd = (end === undefined) ? len : ToInteger(end);

    
    var from = (intStart < 0) ? std_Math_max(len + intStart, 0) : std_Math_min(intStart, len);

    
    var to = (intEnd < 0) ? std_Math_max(len + intEnd, 0) : std_Math_min(intEnd, len);

    
    var span = std_Math_max(to - from, 0);

    
    
    
    
    return SubstringKernel(str, from | 0, span | 0);
}

function String_static_slice(string, start, end) {
    if (arguments.length < 1)
        ThrowTypeError(JSMSG_MISSING_FUN_ARG, 0, 'String.slice');
    return callFunction(String_slice, string, start, end);
}


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
        ThrowRangeError(JSMSG_NEGATIVE_REPETITION_COUNT);

    if (!(n * S.length < (1 << 28)))
        ThrowRangeError(JSMSG_RESULTING_STRING_TOO_LARGE);

    
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

#define STRING_ITERATOR_SLOT_ITERATED_STRING 0
#define STRING_ITERATOR_SLOT_NEXT_INDEX 1


function String_iterator() {
    CheckObjectCoercible(this);
    var S = ToString(this);
    var iterator = NewStringIterator();
    UnsafeSetReservedSlot(iterator, STRING_ITERATOR_SLOT_ITERATED_STRING, S);
    UnsafeSetReservedSlot(iterator, STRING_ITERATOR_SLOT_NEXT_INDEX, 0);
    return iterator;
}

function StringIteratorIdentity() {
    return this;
}

function StringIteratorNext() {
    if (!IsObject(this) || !IsStringIterator(this)) {
        return callFunction(CallStringIteratorMethodIfWrapped, this,
                            "StringIteratorNext");
    }

    var S = UnsafeGetStringFromReservedSlot(this, STRING_ITERATOR_SLOT_ITERATED_STRING);
    
    
    
    var index = UnsafeGetInt32FromReservedSlot(this, STRING_ITERATOR_SLOT_NEXT_INDEX);
    var size = S.length;
    var result = { value: undefined, done: false };

    if (index >= size) {
        result.done = true;
        return result;
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
    result.value = callFunction(std_String_substring, S, index, index + charCount);

    return result;
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


function String_static_fromCodePoint(codePoints) {
    
    
    var length = arguments.length;

    
    var elements = new List();

    
    for (var nextIndex = 0; nextIndex < length; nextIndex++) {
        
        var next = arguments[nextIndex];
        
        var nextCP = ToNumber(next);

        
        if (nextCP !== ToInteger(nextCP) || Number_isNaN(nextCP))
            ThrowRangeError(JSMSG_NOT_A_CODEPOINT, ToString(nextCP));

        
        if (nextCP < 0 || nextCP > 0x10FFFF)
            ThrowRangeError(JSMSG_NOT_A_CODEPOINT, ToString(nextCP));

        
        
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
        ThrowTypeError(JSMSG_MISSING_FUN_ARG, 0, "String.localeCompare");
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
