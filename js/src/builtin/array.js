



 
function ArrayIndexOf(searchElement) {
    
    var O = %ToObject(this);

    
    var len = TO_UINT32(O.length);

    
    if (len === 0)
        return -1;

    
    var n = arguments.length > 1 ? %ToInteger(arguments[1]) : 0;

    
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
        %ThrowError(JSMSG_MISSING_FUN_ARG, 0, 'Array.indexOf');
    var fromIndex = arguments.length > 2 ? arguments[2] : 0;
    return %_CallFunction(list, searchElement, fromIndex, ArrayIndexOf);
}


function ArrayLastIndexOf(searchElement) {
    
    var O = %ToObject(this);

    
    var len = TO_UINT32(O.length);

    
    if (len === 0)
        return -1;

    
    var n = arguments.length > 1 ? %ToInteger(arguments[1]) : len - 1;

    
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
        %ThrowError(JSMSG_MISSING_FUN_ARG, 0, 'Array.lastIndexOf');
    var fromIndex;
    if (arguments.length > 2) {
        fromIndex = arguments[2];
    } else {
        var O = %ToObject(list);
        var len = TO_UINT32(O.length);
        fromIndex = len - 1;
    }
    return %_CallFunction(list, searchElement, fromIndex, ArrayLastIndexOf);
}


function ArrayEvery(callbackfn) {
    
    var O = %ToObject(this);

    
    var len = TO_UINT32(O.length);

    
    if (arguments.length === 0)
        %ThrowError(JSMSG_MISSING_FUN_ARG, 0, 'Array.prototype.every');
    if (!IsCallable(callbackfn))
        %ThrowError(JSMSG_NOT_FUNCTION, callbackfn);

    
    var T = arguments.length > 1 ? arguments[1] : void 0;

    
    
    for (var k = 0; k < len; k++) {
        
        if (k in O) {
            
            if (!%_CallFunction(T, O[k], k, O, callbackfn))
                return false;
        }
    }

    
    return true;
}

function ArrayStaticEvery(list, callbackfn) {
    if (arguments.length < 2)
        %ThrowError(JSMSG_MISSING_FUN_ARG, 0, 'Array.every');
    var T = arguments.length > 2 ? arguments[2] : void 0;
    return %_CallFunction(list, callbackfn, T, ArrayEvery);
}


function ArraySome(callbackfn) {
    
    var O = %ToObject(this);

    
    var len = TO_UINT32(O.length);

    
    if (arguments.length === 0)
        %ThrowError(JSMSG_MISSING_FUN_ARG, 0, 'Array.prototype.some');
    if (!IsCallable(callbackfn))
        %ThrowError(JSMSG_NOT_FUNCTION, callbackfn);

    
    var T = arguments.length > 1 ? arguments[1] : void 0;

    
    
    for (var k = 0; k < len; k++) {
        
        if (k in O) {
            
            if (%_CallFunction(T, O[k], k, O, callbackfn))
                return true;
        }
    }

    
    return false;
}

function ArrayStaticSome(list, callbackfn) {
    if (arguments.length < 2)
        %ThrowError(JSMSG_MISSING_FUN_ARG, 0, 'Array.some');
    var T = arguments.length > 2 ? arguments[2] : void 0;
    return %_CallFunction(list, callbackfn, T, ArraySome);
}


function ArrayForEach(callbackfn) {
    
    var O = %ToObject(this);

    
    var len = TO_UINT32(O.length);

    
    if (arguments.length === 0)
        %ThrowError(JSMSG_MISSING_FUN_ARG, 0, 'Array.prototype.forEach');
    if (!IsCallable(callbackfn))
        %ThrowError(JSMSG_NOT_FUNCTION, callbackfn);

    
    var T = arguments.length > 1 ? arguments[1] : void 0;

    
    
    for (var k = 0; k < len; k++) {
        
        if (k in O) {
            
            %_CallFunction(T, O[k], k, O, callbackfn);
        }
    }

    
    return void 0;
}

function ArrayStaticForEach(list, callbackfn) {
    if (arguments.length < 2)
        %ThrowError(JSMSG_MISSING_FUN_ARG, 0, 'Array.forEach');
    var T = arguments.length > 2 ? arguments[2] : void 0;
    %_CallFunction(list, callbackfn, T, ArrayForEach);
}


function ArrayReduce(callbackfn) {
    
    var O = %ToObject(this);

    
    var len = TO_UINT32(O.length);

    
    if (arguments.length === 0)
        %ThrowError(JSMSG_MISSING_FUN_ARG, 0, 'Array.prototype.reduce');
    if (!IsCallable(callbackfn))
        %ThrowError(JSMSG_NOT_FUNCTION, callbackfn);

    
    var k = 0;

    
    var accumulator;
    if (arguments.length > 1) {
        accumulator = arguments[1];
    } else {
        
        if (len === 0)
            %ThrowError(JSMSG_EMPTY_ARRAY_REDUCE);
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
            %ThrowError(JSMSG_EMPTY_ARRAY_REDUCE);
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
        %ThrowError(JSMSG_MISSING_FUN_ARG, 0, 'Array.reduce');
    if (arguments.length > 2)
        %_CallFunction(list, callbackfn, arguments[2], ArrayReduce);
    else
        %_CallFunction(list, callbackfn, ArrayReduce);
}


function ArrayReduceRight(callbackfn) {
    
    var O = %ToObject(this);

    
    var len = TO_UINT32(O.length);

    
    if (arguments.length === 0)
        %ThrowError(JSMSG_MISSING_FUN_ARG, 0, 'Array.prototype.reduce');
    if (!IsCallable(callbackfn))
        %ThrowError(JSMSG_NOT_FUNCTION, callbackfn);

    
    var k = len - 1;

    
    var accumulator;
    if (arguments.length > 1) {
        accumulator = arguments[1];
    } else {
        
        if (len === 0)
            %ThrowError(JSMSG_EMPTY_ARRAY_REDUCE);
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
            %ThrowError(JSMSG_EMPTY_ARRAY_REDUCE);
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
        %ThrowError(JSMSG_MISSING_FUN_ARG, 0, 'Array.reduceRight');
    if (arguments.length > 2)
        %_CallFunction(list, callbackfn, arguments[2], ArrayReduceRight);
    else
        %_CallFunction(list, callbackfn, ArrayReduceRight);
}
