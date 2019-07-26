






















function toASCIIUpperCase(s) {
    assert(typeof s === "string", "toASCIIUpperCase");

    
    
    
    var result = "";
    for (var i = 0; i < s.length; i++) {
        var c = s[i];
        if ("a" <= c && c <= "z")
            c = callFunction(std_String_toUpperCase, c);
        result += c;
    }
    return result;
}














var unicodeLocaleExtensionSequence = "-u(-[a-z0-9]{2,8})+";
var unicodeLocaleExtensionSequenceRE = new RegExp(unicodeLocaleExtensionSequence);
var unicodeLocaleExtensionSequenceGlobalRE = new RegExp(unicodeLocaleExtensionSequence, "g");







var languageTagRE = (function () {
    
    
    var ALPHA = "[a-zA-Z]";
    
    
    var DIGIT = "[0-9]";

    
    
    var alphanum = "(?:" + ALPHA + "|" + DIGIT + ")";
    
    
    
    
    
    
    
    
    
    var regular = "(?:art-lojban|cel-gaulish|no-bok|no-nyn|zh-guoyu|zh-hakka|zh-min|zh-min-nan|zh-xiang)";
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    var irregular = "(?:en-GB-oed|i-ami|i-bnn|i-default|i-enochian|i-hak|i-klingon|i-lux|i-mingo|i-navajo|i-pwn|i-tao|i-tay|i-tsu|sgn-BE-FR|sgn-BE-NL|sgn-CH-DE)";
    
    
    var grandfathered = "(?:" + irregular + "|" + regular + ")";
    
    var privateuse = "(?:x(?:-[a-z0-9]{1,8})+)";
    
    
    
    
    
    var singleton = "(?:" + DIGIT + "|[A-WY-Za-wy-z])";
    
    var extension = "(?:" + singleton + "(?:-" + alphanum + "{2,8})+)";
    
    
    var variant = "(?:" + alphanum + "{5,8}|(?:" + DIGIT + alphanum + "{3}))";
    
    
    var region = "(?:" + ALPHA + "{2}|" + DIGIT + "{3})";
    
    var script = "(?:" + ALPHA + "{4})";
    
    
    var extlang = "(?:" + ALPHA + "{3}(?:-" + ALPHA + "{3}){0,2})";
    
    
    
    
    
    var language = "(?:" + ALPHA + "{2,3}(?:-" + extlang + ")?|" + ALPHA + "{4}|" + ALPHA + "{5,8})";
    
    
    
    
    
    
    var langtag = language + "(?:-" + script + ")?(?:-" + region + ")?(?:-" +
                  variant + ")*(?:-" + extension + ")*(?:-" + privateuse + ")?";
    
    
    
    var languageTag = "^(?:" + langtag + "|" + privateuse + "|" + grandfathered + ")$";

    
    return new RegExp(languageTag, "i");
}());


var duplicateVariantRE = (function () {
    
    
    var ALPHA = "[a-zA-Z]";
    
    
    var DIGIT = "[0-9]";

    
    
    var alphanum = "(?:" + ALPHA + "|" + DIGIT + ")";
    
    
    var variant = "(?:" + alphanum + "{5,8}|(?:" + DIGIT + alphanum + "{3}))";

    
    var duplicateVariant =
        
        
        
        "(?:" + alphanum + "{2,8}-)+" +
        
        "(" + variant + ")-" +
        
        
        "(?:" + alphanum + "{2,8}-)*" +
        
        "\\1" +
        
        
        "(?!" + alphanum + ")";

    
    
    
    return new RegExp(duplicateVariant);
}());


var duplicateSingletonRE = (function () {
    
    
    var ALPHA = "[a-zA-Z]";
    
    
    var DIGIT = "[0-9]";

    
    
    var alphanum = "(?:" + ALPHA + "|" + DIGIT + ")";
    
    
    
    
    
    var singleton = "(?:" + DIGIT + "|[A-WY-Za-wy-z])";

    
    var duplicateSingleton =
        
        
        "-(" + singleton + ")-" +
        
        "(?:" + alphanum + "+-)*" +
        
        "\\1" +
        
        
        "(?!" + alphanum + ")";

    
    
    
    return new RegExp(duplicateSingleton);
}());








function IsStructurallyValidLanguageTag(locale) {
    assert(typeof locale === "string", "IsStructurallyValidLanguageTag");
    if (!callFunction(std_RegExp_test, languageTagRE, locale))
        return false;

    
    
    
    if (callFunction(std_String_startsWith, locale, "x-"))
        return true;
    var pos = callFunction(std_String_indexOf, locale, "-x-");
    if (pos !== -1)
        locale = callFunction(std_String_substring, locale, 0, pos);

    
    return !callFunction(std_RegExp_test, duplicateVariantRE, locale) &&
           !callFunction(std_RegExp_test, duplicateSingletonRE, locale);
}
