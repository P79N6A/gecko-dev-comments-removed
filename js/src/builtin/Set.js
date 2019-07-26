





function SetForEach(callbackfn, thisArg = undefined) {
    
    var S = this;
    if (!IsObject(S))
        ThrowError(JSMSG_BAD_TYPE, typeof S);

    
    try {
        callFunction(std_Set_has, S);
    } catch (e) {
        ThrowError(JSMSG_BAD_TYPE, typeof S);
    }

    
    if (!IsCallable(callbackfn))
        ThrowError(JSMSG_NOT_FUNCTION, DecompileArg(0, callbackfn));

    
    var values = callFunction(std_Set_iterator, S);
    while (true) {
        var result = callFunction(std_Set_iterator_next, values);
        if (result.done)
            break;
        var value = result.value;
        callFunction(callbackfn, thisArg, value, value, S);
    }
}
