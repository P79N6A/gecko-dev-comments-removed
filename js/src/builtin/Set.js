





function SetForEach(callbackfn, thisArg = undefined) {
    
    var S = this;
    if (!IsObject(S))
        ThrowTypeError(JSMSG_INCOMPATIBLE_PROTO, "Set", "forEach", typeof S);

    
    try {
        callFunction(std_Set_has, S);
    } catch (e) {
        
        ThrowTypeError(JSMSG_INCOMPATIBLE_PROTO, "Set", "forEach", typeof S);
    }

    
    if (!IsCallable(callbackfn))
        ThrowTypeError(JSMSG_NOT_FUNCTION, DecompileArg(0, callbackfn));

    
    var values = callFunction(std_Set_iterator, S);
    while (true) {
        var result = callFunction(std_Set_iterator_next, values);
        if (result.done)
            break;
        var value = result.value;
        callFunction(callbackfn, thisArg, value, value, S);
    }
}
