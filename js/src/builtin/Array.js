



 
function ArrayIndexOf(searchElement) {
    
    var O = ToObject(this);

    
    var len = TO_UINT32(O.length);

    
    if (len === 0)
        return -1;

    
    var n = arguments.length > 1 ? ToInteger(arguments[1]) : 0;

    
    if (n >= len)
        return -1;

    var k;
    
    if (n >= 0)
        k = n;
    
    else {
        
        k = len + n;
        
        if (k < 0)
            k = 0;
    }

    
    for (; k < len; k++) {
        if (k in O && O[k] === searchElement)
            return k;
    }

    
    return -1;
}

function ArrayStaticIndexOf(list, searchElement) {
    if (arguments.length < 1)
        ThrowError(JSMSG_MISSING_FUN_ARG, 0, 'Array.indexOf');
    var fromIndex = arguments.length > 2 ? arguments[2] : 0;
    return callFunction(ArrayIndexOf, list, searchElement, fromIndex);
}


function ArrayLastIndexOf(searchElement) {
    
    var O = ToObject(this);

    
    var len = TO_UINT32(O.length);

    
    if (len === 0)
        return -1;

    
    var n = arguments.length > 1 ? ToInteger(arguments[1]) : len - 1;

    
    var k;
    if (n > len - 1)
        k = len - 1;
    else if (n < 0)
        k = len + n;
    else
        k = n;

    
    for (; k >= 0; k--) {
        if (k in O && O[k] === searchElement)
            return k;
    }

    
    return -1;
}

function ArrayStaticLastIndexOf(list, searchElement) {
    if (arguments.length < 1)
        ThrowError(JSMSG_MISSING_FUN_ARG, 0, 'Array.lastIndexOf');
    var fromIndex;
    if (arguments.length > 2) {
        fromIndex = arguments[2];
    } else {
        var O = ToObject(list);
        var len = TO_UINT32(O.length);
        fromIndex = len - 1;
    }
    return callFunction(ArrayLastIndexOf, list, searchElement, fromIndex);
}


function ArrayEvery(callbackfn) {
    
    var O = ToObject(this);

    
    var len = TO_UINT32(O.length);

    
    if (arguments.length === 0)
        ThrowError(JSMSG_MISSING_FUN_ARG, 0, 'Array.prototype.every');
    if (!IsCallable(callbackfn))
        ThrowError(JSMSG_NOT_FUNCTION, DecompileArg(0, callbackfn));

    
    var T = arguments.length > 1 ? arguments[1] : void 0;

    
    
    for (var k = 0; k < len; k++) {
        
        if (k in O) {
            
            if (!callFunction(callbackfn, T, O[k], k, O))
                return false;
        }
    }

    
    return true;
}

function ArrayStaticEvery(list, callbackfn) {
    if (arguments.length < 2)
        ThrowError(JSMSG_MISSING_FUN_ARG, 0, 'Array.every');
    if (!IsCallable(callbackfn))
        ThrowError(JSMSG_NOT_FUNCTION, DecompileArg(1, callbackfn));
    var T = arguments.length > 2 ? arguments[2] : void 0;
    return callFunction(ArrayEvery, list, callbackfn, T);
}


function ArraySome(callbackfn) {
    
    var O = ToObject(this);

    
    var len = TO_UINT32(O.length);

    
    if (arguments.length === 0)
        ThrowError(JSMSG_MISSING_FUN_ARG, 0, 'Array.prototype.some');
    if (!IsCallable(callbackfn))
        ThrowError(JSMSG_NOT_FUNCTION, DecompileArg(0, callbackfn));

    
    var T = arguments.length > 1 ? arguments[1] : void 0;

    
    
    for (var k = 0; k < len; k++) {
        
        if (k in O) {
            
            if (callFunction(callbackfn, T, O[k], k, O))
                return true;
        }
    }

    
    return false;
}

function ArrayStaticSome(list, callbackfn) {
    if (arguments.length < 2)
        ThrowError(JSMSG_MISSING_FUN_ARG, 0, 'Array.some');
    if (!IsCallable(callbackfn))
        ThrowError(JSMSG_NOT_FUNCTION, DecompileArg(1, callbackfn));
    var T = arguments.length > 2 ? arguments[2] : void 0;
    return callFunction(ArraySome, list, callbackfn, T);
}


function ArrayForEach(callbackfn) {
    
    var O = ToObject(this);

    
    var len = TO_UINT32(O.length);

    
    if (arguments.length === 0)
        ThrowError(JSMSG_MISSING_FUN_ARG, 0, 'Array.prototype.forEach');
    if (!IsCallable(callbackfn))
        ThrowError(JSMSG_NOT_FUNCTION, DecompileArg(0, callbackfn));

    
    var T = arguments.length > 1 ? arguments[1] : void 0;

    
    
    for (var k = 0; k < len; k++) {
        
        if (k in O) {
            
            callFunction(callbackfn, T, O[k], k, O);
        }
    }

    
    return void 0;
}


function ArrayMap(callbackfn) {
    
    var O = ToObject(this);

    
    var len = TO_UINT32(O.length);

    
    if (arguments.length === 0)
        ThrowError(JSMSG_MISSING_FUN_ARG, 0, 'Array.prototype.map');
    if (!IsCallable(callbackfn))
        ThrowError(JSMSG_NOT_FUNCTION, DecompileArg(0, callbackfn));

    
    var T = arguments.length > 1 ? arguments[1] : void 0;

    
    var A = NewDenseArray(len);

    
    
    for (var k = 0; k < len; k++) {
        
        if (k in O) {
            
            var mappedValue = callFunction(callbackfn, T, O[k], k, O);
            
            UnsafeSetElement(A, k, mappedValue);
        }
    }

    
    return A;
}

function ArrayStaticMap(list, callbackfn) {
    if (arguments.length < 2)
        ThrowError(JSMSG_MISSING_FUN_ARG, 0, 'Array.map');
    if (!IsCallable(callbackfn))
        ThrowError(JSMSG_NOT_FUNCTION, DecompileArg(1, callbackfn));
    var T = arguments.length > 2 ? arguments[2] : void 0;
    return callFunction(ArrayMap, list, callbackfn, T);
}

function ArrayStaticForEach(list, callbackfn) {
    if (arguments.length < 2)
        ThrowError(JSMSG_MISSING_FUN_ARG, 0, 'Array.forEach');
    if (!IsCallable(callbackfn))
        ThrowError(JSMSG_NOT_FUNCTION, DecompileArg(1, callbackfn));
    var T = arguments.length > 2 ? arguments[2] : void 0;
    callFunction(ArrayForEach, list, callbackfn, T);
}


function ArrayReduce(callbackfn) {
    
    var O = ToObject(this);

    
    var len = TO_UINT32(O.length);

    
    if (arguments.length === 0)
        ThrowError(JSMSG_MISSING_FUN_ARG, 0, 'Array.prototype.reduce');
    if (!IsCallable(callbackfn))
        ThrowError(JSMSG_NOT_FUNCTION, DecompileArg(0, callbackfn));

    
    var k = 0;

    
    var accumulator;
    if (arguments.length > 1) {
        accumulator = arguments[1];
    } else {
        
        if (len === 0)
            ThrowError(JSMSG_EMPTY_ARRAY_REDUCE);
        var kPresent = false;
        for (; k < len; k++) {
            if (k in O) {
                accumulator = O[k];
                kPresent = true;
                k++;
                break;
            }
        }
        if (!kPresent)
            ThrowError(JSMSG_EMPTY_ARRAY_REDUCE);
    }

    
    
    for (; k < len; k++) {
        
        if (k in O) {
            
            accumulator = callbackfn(accumulator, O[k], k, O);
        }
    }

    
    return accumulator;
}

function ArrayStaticReduce(list, callbackfn) {
    if (arguments.length < 2)
        ThrowError(JSMSG_MISSING_FUN_ARG, 0, 'Array.reduce');
    if (!IsCallable(callbackfn))
        ThrowError(JSMSG_NOT_FUNCTION, DecompileArg(1, callbackfn));
    if (arguments.length > 2)
        return callFunction(ArrayReduce, list, callbackfn, arguments[2]);
    else
        return callFunction(ArrayReduce, list, callbackfn);
}


function ArrayReduceRight(callbackfn) {
    
    var O = ToObject(this);

    
    var len = TO_UINT32(O.length);

    
    if (arguments.length === 0)
        ThrowError(JSMSG_MISSING_FUN_ARG, 0, 'Array.prototype.reduce');
    if (!IsCallable(callbackfn))
        ThrowError(JSMSG_NOT_FUNCTION, DecompileArg(0, callbackfn));

    
    var k = len - 1;

    
    var accumulator;
    if (arguments.length > 1) {
        accumulator = arguments[1];
    } else {
        
        if (len === 0)
            ThrowError(JSMSG_EMPTY_ARRAY_REDUCE);
        var kPresent = false;
        for (; k >= 0; k--) {
            if (k in O) {
                accumulator = O[k];
                kPresent = true;
                k--;
                break;
            }
        }
        if (!kPresent)
            ThrowError(JSMSG_EMPTY_ARRAY_REDUCE);
    }

    
    
    for (; k >= 0; k--) {
        
        if (k in O) {
            
            accumulator = callbackfn(accumulator, O[k], k, O);
        }
    }

    
    return accumulator;
}

function ArrayStaticReduceRight(list, callbackfn) {
    if (arguments.length < 2)
        ThrowError(JSMSG_MISSING_FUN_ARG, 0, 'Array.reduceRight');
    if (!IsCallable(callbackfn))
        ThrowError(JSMSG_NOT_FUNCTION, DecompileArg(1, callbackfn));
    if (arguments.length > 2)
        return callFunction(ArrayReduceRight, list, callbackfn, arguments[2]);
    else
        return callFunction(ArrayReduceRight, list, callbackfn);
}
