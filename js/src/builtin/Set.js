





function SetForEach(callbackfn, thisArg = undefined) {
    
    var S = this;
    if (!IsObject(S))
        ThrowError(JSMSG_BAD_TYPE, typeof S);

    
    try {
        std_Set_has.call(S);
    } catch (e) {
        ThrowError(JSMSG_BAD_TYPE, typeof S);
    }

    
    if (!IsCallable(callbackfn))
        ThrowError(JSMSG_NOT_FUNCTION, DecompileArg(0, callbackfn));

    
    var values = std_Set_iterator.call(S);
    while (true) {
        try {
            var entry = std_Set_iterator_next.call(values);
        } catch (err) {
            if (err instanceof StopIteration)
                break;
            throw err;
        }
        callFunction(callbackfn, thisArg, entry, entry, S);
    }
}
