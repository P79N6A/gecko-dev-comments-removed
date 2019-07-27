




function RegExpFlagsGetter() {
    
    var R = this;
    if (!IsObject(R))
        ThrowError(JSMSG_NOT_NONNULL_OBJECT, R === null ? "null" : typeof R);

    
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
