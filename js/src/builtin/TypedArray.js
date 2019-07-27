




function TypedArrayFill(value, start = 0, end = undefined) {
    
    if (!IsObject(this) || !IsTypedArray(this)) {
        return callFunction(CallTypedArrayMethodIfWrapped, this, value, start, end,
                            "TypedArrayFill");
    }

    
    var O = this;

    
    var len = TypedArrayLength(O);

    
    var relativeStart = ToInteger(start);

    
    var k = relativeStart < 0
            ? std_Math_max(len + relativeStart, 0)
            : std_Math_min(relativeStart, len);

    
    var relativeEnd = end === undefined ? len : ToInteger(end);

    
    var final = relativeEnd < 0
                ? std_Math_max(len + relativeEnd, 0)
                : std_Math_min(relativeEnd, len);

    
    for (; k < final; k++) {
        O[k] = value;
    }

    
    return O;
}


function TypedArrayFind(predicate, thisArg = undefined) {
    
    if (!IsObject(this) || !IsTypedArray(this)) {
        return callFunction(CallTypedArrayMethodIfWrapped, this, predicate, thisArg,
                            "TypedArrayFind");
    }

    
    var O = this;

    
    var len = TypedArrayLength(O);

    
    if (arguments.length === 0)
        ThrowError(JSMSG_MISSING_FUN_ARG, 0, "%TypedArray%.prototype.find");
    if (!IsCallable(predicate))
        ThrowError(JSMSG_NOT_FUNCTION, DecompileArg(0, predicate));

    
    var T = thisArg;

    
    
    for (var k = 0; k < len; k++) {
        
        var kValue = O[k];
        
        if (callFunction(predicate, T, kValue, k, O))
            return kValue;
    }

    
    return undefined;
}


function TypedArrayFindIndex(predicate, thisArg = undefined) {
    
    if (!IsObject(this) || !IsTypedArray(this)) {
        return callFunction(CallTypedArrayMethodIfWrapped, this, predicate, thisArg,
                            "TypedArrayFindIndex");
    }

    
    var O = this;

    
    var len = TypedArrayLength(O);

    
    if (arguments.length === 0)
        ThrowError(JSMSG_MISSING_FUN_ARG, 0, "%TypedArray%.prototype.findIndex");
    if (!IsCallable(predicate))
        ThrowError(JSMSG_NOT_FUNCTION, DecompileArg(0, predicate));

    
    var T = thisArg;

    
    
    for (var k = 0; k < len; k++) {
        
        if (callFunction(predicate, T, O[k], k, O))
            return k;
    }

    
    return -1;
}


function TypedArrayIndexOf(searchElement, fromIndex = 0) {
    
    if (!IsObject(this) || !IsTypedArray(this)) {
        return callFunction(CallTypedArrayMethodIfWrapped, this, searchElement, fromIndex,
                            "TypedArrayIndexOf");
    }

    
    var O = this;

    
    var len = TypedArrayLength(O);

    
    if (len === 0)
        return -1;

    
    var n = ToInteger(fromIndex);

    
    if (n >= len)
        return -1;

    var k;
    
    if (n >= 0) {
        k = n;
    }
    
    else {
        
        k = len + n;
        
        if (k < 0)
            k = 0;
    }

    
    
    for (; k < len; k++) {
        if (O[k] === searchElement)
            return k;
    }

    
    return -1;
}


function TypedArrayJoin(separator) {
    
    if (!IsObject(this) || !IsTypedArray(this)) {
        return callFunction(CallTypedArrayMethodIfWrapped, this, separator, "TypedArrayJoin");
    }

    
    var O = this;

    
    var len = TypedArrayLength(O);

    
    var sep = separator === undefined ? "," : ToString(separator);

    
    if (len === 0)
        return "";

    
    var element0 = O[0];

    
    
    var R = ToString(element0);

    
    for (var k = 1; k < len; k++) {
        
        var S = R + sep;

        
        var element = O[k];

        
        
        var next = ToString(element);

        
        R = S + next;
    }

    
    return R;
}


function TypedArrayLastIndexOf(searchElement, fromIndex = undefined) {
    
    if (!IsObject(this) || !IsTypedArray(this)) {
        return callFunction(CallTypedArrayMethodIfWrapped, this, searchElement, fromIndex,
                            "TypedArrayLastIndexOf");
    }

    
    var O = this;

    
    var len = TypedArrayLength(O);

    
    if (len === 0)
        return -1;

    
    var n = fromIndex === undefined ? len - 1 : ToInteger(fromIndex);

    
    var k = n >= 0 ? std_Math_min(n, len - 1) : len + n;

    
    
    for (; k >= 0; k--) {
        if (O[k] === searchElement)
            return k;
    }

    
    return -1;
}


function TypedArrayReverse() {
    
    if (!IsObject(this) || !IsTypedArray(this)) {
        return callFunction(CallTypedArrayMethodIfWrapped, this, "TypedArrayReverse");
    }

    
    var O = this;

    
    var len = TypedArrayLength(O);

    
    var middle = std_Math_floor(len / 2);

    
    
    
    for (var lower = 0; lower !== middle; lower++) {
        
        var upper = len - lower - 1;

        
        var lowerValue = O[lower];

        
        var upperValue = O[upper];

        
        O[lower] = upperValue;
        O[upper] = lowerValue;
    }

    
    return O;
}



function TypedArrayIncludes(searchElement, fromIndex = 0) {
    
    if (!IsObject(this) || !IsTypedArray(this)) {
        return callFunction(CallTypedArrayMethodIfWrapped, this, searchElement,
                            fromIndex, "TypedArrayIncludes");
    }

    
    var O = this;

    
    var len = TypedArrayLength(O);

    
    if (len === 0)
        return false;

    
    var n = ToInteger(fromIndex);

    var k;
    
    if (n >= 0) {
        k = n;
    }
    
    else {
        
        k = len + n;
        
        if (k < 0)
            k = 0;
    }

    
    while (k < len) {
        
        if (SameValueZero(searchElement, O[k]))
            return true;

        
        k++;
    }

    
    return false;
}
