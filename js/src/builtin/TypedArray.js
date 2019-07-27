




function TypedArrayFind(predicate, thisArg = undefined) {
    
    if (!IsObject(this) || !IsTypedArray(this))
        ThrowError(JSMSG_INCOMPATIBLE_PROTO, "%TypedArray%", "find", typeof this);

    
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
    
    if (!IsObject(this) || !IsTypedArray(this))
        ThrowError(JSMSG_INCOMPATIBLE_PROTO, "%TypedArray%", "findIndex", typeof this);

    
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
