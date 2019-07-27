





function MapForEach(callbackfn, thisArg = undefined) {
    
    var M = this;
    if (!IsObject(M))
        ThrowTypeError(JSMSG_INCOMPATIBLE_PROTO, "Map", "forEach", typeof M);

    
    try {
        callFunction(std_Map_has, M);
    } catch (e) {
        
        ThrowTypeError(JSMSG_INCOMPATIBLE_PROTO, "Map", "forEach", typeof M);
    }

    
    if (!IsCallable(callbackfn))
        ThrowTypeError(JSMSG_NOT_FUNCTION, DecompileArg(0, callbackfn));

    
    var entries = callFunction(std_Map_iterator, M);
    while (true) {
        var result = callFunction(std_Map_iterator_next, entries);
        if (result.done)
            break;
        var entry = result.value;
        callFunction(callbackfn, thisArg, entry[1], entry[0], M);
    }
}
