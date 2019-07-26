





function MapForEach(callbackfn, thisArg = undefined) {
    
    var M = this;
    if (!IsObject(M))
        ThrowError(JSMSG_BAD_TYPE, typeof M);

    
    try {
        std_Map_has.call(M);
    } catch (e) {
        ThrowError(JSMSG_BAD_TYPE, typeof M);
    }

    
    if (!IsCallable(callbackfn))
        ThrowError(JSMSG_NOT_FUNCTION, DecompileArg(0, callbackfn));

    
    var entries = std_Map_iterator.call(M);
    while (true) {
        try {
            var entry = std_Map_iterator_next.call(entries);
        } catch (err) {
            if (err instanceof StopIteration)
                break;
            throw err;
        }
        callFunction(callbackfn, thisArg, entry[1], entry[0], M);
    }
}
