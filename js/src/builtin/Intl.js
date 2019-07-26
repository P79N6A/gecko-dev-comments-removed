






















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






















function CanonicalizeLanguageTag(locale) {
    assert(IsStructurallyValidLanguageTag(locale), "CanonicalizeLanguageTag");

    
    
    

    
    
    
    
    
    
    locale = callFunction(std_String_toLowerCase, locale);

    
    if (callFunction(std_Object_hasOwnProperty, langTagMappings, locale))
        return langTagMappings[locale];

    var subtags = callFunction(std_String_split, locale, "-");
    var i = 0;

    
    
    while (i < subtags.length) {
        var subtag = subtags[i];

        
        
        
        
        
        if (subtag.length === 1 && (i > 0 || subtag === "x"))
            break;

        if (subtag.length === 4) {
            
            
            subtag = callFunction(std_String_toUpperCase, subtag[0]) +
                     callFunction(std_String_substring, subtag, 1);
        } else if (i !== 0 && subtag.length === 2) {
            
            
            subtag = callFunction(std_String_toUpperCase, subtag);
        }
        if (callFunction(std_Object_hasOwnProperty, langSubtagMappings, subtag)) {
            
            
            
            
            
            
            
            
            subtag = langSubtagMappings[subtag];
        } else if (callFunction(std_Object_hasOwnProperty, extlangMappings, subtag)) {
            
            
            
            
            
            subtag = extlangMappings[subtag].preferred;
            if (i === 1 && extlangMappings[subtag].prefix === subtags[0]) {
                callFunction(std_Array_shift, subtags);
                i--;
            }
        }
        subtags[i] = subtag;
        i++;
    }
    var normal = callFunction(std_Array_join, callFunction(std_Array_slice, subtags, 0, i), "-");

    
    
    var extensions = new List();
    while (i < subtags.length && subtags[i] !== "x") {
        var extensionStart = i;
        i++;
        while (i < subtags.length && subtags[i].length > 1)
            i++;
        var extension = callFunction(std_Array_join, callFunction(std_Array_slice, subtags, extensionStart, i), "-");
        extensions.push(extension);
    }
    extensions.sort();

    
    var privateUse = "";
    if (i < subtags.length)
        privateUse = callFunction(std_Array_join, callFunction(std_Array_slice, subtags, i), "-");

    
    var canonical = normal;
    if (extensions.length > 0)
        canonical += "-" + extensions.join("-");
    if (privateUse.length > 0) {
        
        if (canonical.length > 0)
            canonical += "-" + privateUse;
        else
            canonical = privateUse;
    }

    return canonical;
}







function IsWellFormedCurrencyCode(currency) {
    var c = ToString(currency);
    var normalized = toASCIIUpperCase(c);
    if (normalized.length !== 3)
        return false;
    return !callFunction(std_RegExp_test, /[^A-Z]/, normalized);
}












function addOldStyleLanguageTags(availableLocales) {
    
    if (availableLocales["pa-Arab-PK"])
        availableLocales["pa-PK"] = true;
    if (availableLocales["zh-Hans-CN"])
        availableLocales["zh-CN"] = true;
    if (availableLocales["zh-Hans-SG"])
        availableLocales["zh-SG"] = true;
    if (availableLocales["zh-Hant-HK"])
        availableLocales["zh-HK"] = true;
    if (availableLocales["zh-Hant-TW"])
        availableLocales["zh-TW"] = true;
    return availableLocales;
}







function CanonicalizeLocaleList(locales) {
    if (locales === undefined)
        return new List();
    var seen = new List();
    if (typeof locales === "string")
        locales = [locales];
    var O = ToObject(locales);
    var len = TO_UINT32(O.length);
    var k = 0;
    while (k < len) {
        
        var kPresent = HasProperty(O, k);
        if (kPresent) {
            var kValue = O[k];
            if (!(typeof kValue === "string" || IsObject(kValue)))
                ThrowError(JSMSG_INVALID_LOCALES_ELEMENT);
            var tag = ToString(kValue);
            if (!IsStructurallyValidLanguageTag(tag))
                ThrowError(JSMSG_INVALID_LANGUAGE_TAG, tag);
            tag = CanonicalizeLanguageTag(tag);
            if (seen.indexOf(tag) === -1)
                seen.push(tag);
        }
        k++;
    }
    return seen;
}










function BestAvailableLocale(availableLocales, locale) {
    assert(IsStructurallyValidLanguageTag(locale), "BestAvailableLocale");
    assert(locale === CanonicalizeLanguageTag(locale), "BestAvailableLocale");
    assert(callFunction(std_String_indexOf, locale, "-u-") === -1, "BestAvailableLocale");

    var candidate = locale;
    while (true) {
        if (availableLocales[candidate])
            return candidate;
        var pos = callFunction(std_String_lastIndexOf, candidate, "-");
        if (pos === -1)
            return undefined;
        if (pos >= 2 && candidate[pos - 2] === "-")
            pos -= 2;
        candidate = callFunction(std_String_substring, candidate, 0, pos);
    }
}














function LookupMatcher(availableLocales, requestedLocales) {
    var i = 0;
    var len = requestedLocales.length;
    var availableLocale;
    var locale, noExtensionsLocale;
    while (i < len && availableLocale === undefined) {
        locale = requestedLocales[i];
        noExtensionsLocale = callFunction(std_String_replace, locale, unicodeLocaleExtensionSequenceGlobalRE, "");
        availableLocale = BestAvailableLocale(availableLocales, noExtensionsLocale);
        i++;
    }

    var result = new Record();
    if (availableLocale !== undefined) {
        result.__locale = availableLocale;
        if (locale !== noExtensionsLocale) {
            var extensionMatch = callFunction(std_String_match, locale, unicodeLocaleExtensionSequenceRE);
            var extension = extensionMatch[0];
            var extensionIndex = extensionMatch.index;
            result.__extension = extension;
            result.__extensionIndex = extensionIndex;
        }
    } else {
        result.__locale = DefaultLocale();
    }
    return result;
}











function BestFitMatcher(availableLocales, requestedLocales) {
    
    return LookupMatcher(availableLocales, requestedLocales);
}
