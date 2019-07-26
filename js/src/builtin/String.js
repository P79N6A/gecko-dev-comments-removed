






var collatorCache = new Record();


function String_repeat(count) {
    
    CheckObjectCoercible(this);
    var S = ToString(this);

    
    var n = ToInteger(count);

    
    if (n < 0 || n === std_Number_POSITIVE_INFINITY)
        ThrowError(JSMSG_REPEAT_RANGE);

    
    if (n === 0)
        return "";

    var result = S;
    for (var i = 1; i <= n / 2; i *= 2)
        result += result;
    for (; i < n; i++)
        result += S;
    return result;
}







function String_localeCompare(that) {
    
    CheckObjectCoercible(this);
    var S = ToString(this);
    var That = ToString(that);

    
    var locales = arguments.length > 1 ? arguments[1] : undefined;
    var options = arguments.length > 2 ? arguments[2] : undefined;

    
    var collator;
    if (locales === undefined && options === undefined) {
        
        
        if (collatorCache.collator === undefined)
            collatorCache.collator = intl_Collator(locales, options);
        collator = collatorCache.collator;
    } else {
        collator = intl_Collator(locales, options);
    }

    
    return intl_CompareStrings(collator, S, That);
}








function String_static_localeCompare(str1, str2) {
    if (arguments.length < 1)
        ThrowError(JSMSG_MISSING_FUN_ARG, 0, "String.localeCompare");
    var locales = arguments.length > 2 ? arguments[2] : undefined;
    var options = arguments.length > 3 ? arguments[3] : undefined;
    return callFunction(String_localeCompare, str1, str2, locales, options);
}
