




function RegExpFlagsGetter() {
    
    var R = this;
    if (!IsObject(R))
        ThrowTypeError(JSMSG_NOT_NONNULL_OBJECT, R === null ? "null" : typeof R);

    
    var result = "";

    
    if (R.global)
        result += "g";

    
    if (R.ignoreCase)
        result += "i";

    
    if (R.multiline)
        result += "m";

    
    
    
    

    
    if (R.sticky)
        result += "y";

    
    return result;
}


function RegExpToString()
{
    
    var R = this;
    if (!IsObject(R))
        ThrowTypeError(JSMSG_NOT_NONNULL_OBJECT, R === null ? "null" : typeof R);

    
    var pattern = R.source;

    
    var flags = R.flags;

    
    return '/' + pattern + '/' + flags;
}
