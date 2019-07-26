






















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




var oldStyleLanguageTagMappings = {
    "pa-PK": "pa-Arab-PK",
    "zh-CN": "zh-Hans-CN",
    "zh-HK": "zh-Hant-HK",
    "zh-SG": "zh-Hans-SG",
    "zh-TW": "zh-Hant-TW"
};







function DefaultLocale() {
    var localeOfLastResort = "und";

    var locale = RuntimeDefaultLocale();
    if (!IsStructurallyValidLanguageTag(locale))
        return localeOfLastResort;

    locale = CanonicalizeLanguageTag(locale);
    if (callFunction(std_Object_hasOwnProperty, oldStyleLanguageTagMappings, locale))
        locale = oldStyleLanguageTagMappings[locale];

    if (!(collatorInternalProperties.availableLocales[locale] &&
          numberFormatInternalProperties.availableLocales[locale] &&
          dateTimeFormatInternalProperties.availableLocales[locale]))
    {
        locale = localeOfLastResort;
    }
    return locale;
}







function IsWellFormedCurrencyCode(currency) {
    var c = ToString(currency);
    var normalized = toASCIIUpperCase(c);
    if (normalized.length !== 3)
        return false;
    return !callFunction(std_RegExp_test, /[^A-Z]/, normalized);
}












function addOldStyleLanguageTags(availableLocales) {
    var oldStyleLocales = std_Object_getOwnPropertyNames(oldStyleLanguageTagMappings);
    for (var i = 0; i < oldStyleLocales.length; i++) {
        var oldStyleLocale = oldStyleLocales[i];
        if (availableLocales[oldStyleLanguageTagMappings[oldStyleLocale]])
            availableLocales[oldStyleLocale] = true;
    }
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
        result.locale = availableLocale;
        if (locale !== noExtensionsLocale) {
            var extensionMatch = callFunction(std_String_match, locale, unicodeLocaleExtensionSequenceRE);
            var extension = extensionMatch[0];
            var extensionIndex = extensionMatch.index;
            result.extension = extension;
            result.extensionIndex = extensionIndex;
        }
    } else {
        result.locale = DefaultLocale();
    }
    return result;
}











function BestFitMatcher(availableLocales, requestedLocales) {
    
    return LookupMatcher(availableLocales, requestedLocales);
}











function ResolveLocale(availableLocales, requestedLocales, options, relevantExtensionKeys, localeData) {
    

    
    var matcher = options.localeMatcher;
    var r = (matcher === "lookup")
            ? LookupMatcher(availableLocales, requestedLocales)
            : BestFitMatcher(availableLocales, requestedLocales);

    
    var foundLocale = r.locale;

    
    var extension = r.extension;
    var extensionIndex, extensionSubtags, extensionSubtagsLength;

    
    if (extension !== undefined) {
        
        extensionIndex = r.extensionIndex;

        
        extensionSubtags = callFunction(std_String_split, extension, "-");
        extensionSubtagsLength = extensionSubtags.length;
    }

    
    var result = new Record();
    result.dataLocale = foundLocale;

    
    var supportedExtension = "-u";

    
    var i = 0;
    var len = relevantExtensionKeys.length;
    while (i < len) {
        
        var key = relevantExtensionKeys[i];

        
        var foundLocaleData = localeData(foundLocale);
        var keyLocaleData = foundLocaleData[key];

        
        
        var value = keyLocaleData[0];

        

        
        var supportedExtensionAddition = "";

        

        var valuePos;

        
        if (extensionSubtags !== undefined) {
            
            var keyPos = callFunction(std_Array_indexOf, extensionSubtags, key);

            
            if (keyPos !== -1) {
                
                if (keyPos + 1 < extensionSubtagsLength &&
                    extensionSubtags[keyPos + 1].length > 2)
                {
                    
                    var requestedValue = extensionSubtags[keyPos + 1];

                    
                    valuePos = callFunction(std_Array_indexOf, keyLocaleData, requestedValue);

                    
                    if (valuePos !== -1) {
                        value = requestedValue;
                        supportedExtensionAddition = "-" + key + "-" + value;
                    }
                } else {
                    

                    
                    

                    
                    valuePos = callFunction(std_Array_indexOf, keyLocaleData, "true");

                    
                    if (valuePos !== -1)
                        value = "true";
                }
            }
        }

        

        
        var optionsValue = options[key];

        
        if (optionsValue !== undefined &&
            callFunction(std_Array_indexOf, keyLocaleData, optionsValue) !== -1)
        {
            
            if (optionsValue !== value) {
                value = optionsValue;
                supportedExtensionAddition = "";
            }
        }

        
        result[key] = value;
        supportedExtension += supportedExtensionAddition;
        i++;
    }

    
    if (supportedExtension.length > 2) {
        var preExtension = callFunction(std_String_substring, foundLocale, 0, extensionIndex);
        var postExtension = callFunction(std_String_substring, foundLocale, extensionIndex);
        foundLocale = preExtension + supportedExtension + postExtension;
    }

    
    result.locale = foundLocale;
    return result;
}









function LookupSupportedLocales(availableLocales, requestedLocales) {
    
    var len = requestedLocales.length;
    var subset = new List();

    
    var k = 0;
    while (k < len) {
        
        var locale = requestedLocales[k];
        var noExtensionsLocale = callFunction(std_String_replace, locale, unicodeLocaleExtensionSequenceGlobalRE, "");

        
        var availableLocale = BestAvailableLocale(availableLocales, noExtensionsLocale);
        if (availableLocale !== undefined)
            subset.push(locale);

        
        k++;
    }

    
    return subset.slice(0);
}









function BestFitSupportedLocales(availableLocales, requestedLocales) {
    
    return LookupSupportedLocales(availableLocales, requestedLocales);
}









function SupportedLocales(availableLocales, requestedLocales, options) {
    

    
    var matcher;
    if (options !== undefined) {
        
        options = ToObject(options);
        matcher = options.localeMatcher;

        
        if (matcher !== undefined) {
            matcher = ToString(matcher);
            if (matcher !== "lookup" && matcher !== "best fit")
                ThrowError(JSMSG_INVALID_LOCALE_MATCHER, matcher);
        }
    }

    
    var subset = (matcher === undefined || matcher === "best fit")
                 ? BestFitSupportedLocales(availableLocales, requestedLocales)
                 : LookupSupportedLocales(availableLocales, requestedLocales);

    
    for (var i = 0; i < subset.length; i++)
        std_Object_defineProperty(subset, i, {value: subset[i], writable: false, enumerable: true, configurable: false});



    
    return subset;
}









function GetOption(options, property, type, values, fallback) {
    
    var value = options[property];

    
    if (value !== undefined) {
        
        if (type === "boolean")
            value = ToBoolean(value);
        else if (type === "string")
            value = ToString(value);
        else
            assert(false, "GetOption");

        
        if (values !== undefined && callFunction(std_Array_indexOf, values, value) === -1)
            ThrowError(JSMSG_INVALID_OPTION_VALUE, property, value);

        
        return value;
    }

    
    return fallback;
}








function GetNumberOption(options, property, minimum, maximum, fallback) {
    assert(typeof minimum === "number", "GetNumberOption");
    assert(typeof maximum === "number", "GetNumberOption");
    assert(fallback === undefined || (fallback >= minimum && fallback <= maximum), "GetNumberOption");

    
    var value = options[property];

    
    if (value !== undefined) {
        value = ToNumber(value);
        if (std_isNaN(value) || value < minimum || value > maximum)
            ThrowError(JSMSG_INVALID_DIGITS_VALUE, value);
        return std_Math_floor(value);
    }

    
    return fallback;
}



var runtimeAvailableLocales = (function () {
    var o = std_Object_create(null);
    o[RuntimeDefaultLocale()] = true;
    return addOldStyleLanguageTags(o);
}());









function defineProperty(o, p, v) {
    std_Object_defineProperty(o, p, {value: v, writable: true, enumerable: true, configurable: true});
}











var internalsMap = new WeakMap();







function initializeIntlObject(o) {
    assert(IsObject(o), "initializeIntlObject");
    var internals = std_Object_create(null);
    callFunction(std_WeakMap_set, internalsMap, o, internals);
    return internals;
}







function isInitializedIntlObject(o) {
    return callFunction(std_WeakMap_has, internalsMap, o);
}





function getInternals(o) {
    return callFunction(std_WeakMap_get, internalsMap, o);
}












function checkIntlAPIObject(o, className, methodName) {
    assert(typeof className === "string", "checkIntlAPIObject");
    var internals = getInternals(o);
    if (internals === undefined || internals["initialized" + className] !== true)
        ThrowError(JSMSG_INTL_OBJECT_NOT_INITED, className, methodName, className);
    assert(IsObject(o), "checkIntlAPIObject");
    return internals;
}











var collatorKeyMappings = {
    kn: {property: "numeric", type: "boolean"},
    kf: {property: "caseFirst", type: "string", values: ["upper", "lower", "false"]}
};







function InitializeCollator(collator, locales, options) {
    assert(IsObject(collator), "InitializeCollator");

    
    if (isInitializedIntlObject(collator))
        ThrowError(JSMSG_INTL_OBJECT_REINITED);

    
    var internals = initializeIntlObject(collator);

    
    var requestedLocales = CanonicalizeLocaleList(locales);

    
    if (options === undefined)
        options = {};
    else
        options = ToObject(options);

    
    
    var u = GetOption(options, "usage", "string", ["sort", "search"], "sort");
    internals.usage = u;

    
    var Collator = collatorInternalProperties;

    
    var localeData = u === "sort" ? Collator.sortLocaleData : Collator.searchLocaleData;

    
    var opt = new Record();

    
    var matcher = GetOption(options, "localeMatcher", "string", ["lookup", "best fit"], "best fit");
    opt.localeMatcher = matcher;

    
    
    var key, mapping, property, value;
    for (key in collatorKeyMappings) {
        if (callFunction(std_Object_hasOwnProperty, collatorKeyMappings, key)) {
            mapping = collatorKeyMappings[key];

            
            value = GetOption(options, mapping.property, mapping.type, mapping.values, undefined);

            
            if (mapping.type === "boolean" && value !== undefined)
                value = callFunction(std_Boolean_toString, value);

            
            opt[key] = value;
        }
    }

    
    
    var relevantExtensionKeys = Collator.relevantExtensionKeys;

    
    var r = ResolveLocale(Collator.availableLocales,
                          requestedLocales, opt,
                          relevantExtensionKeys,
                          localeData);
    
    internals.locale = r.locale;

    
    var i = 0, len = relevantExtensionKeys.length;
    while (i < len) {
        
        key = relevantExtensionKeys[i];
        if (key === "co") {
            
            property = "collation";
            value = r.co === null ? "default" : r.co;
        } else {
            
            mapping = collatorKeyMappings[key];
            property = mapping.property;
            value = r[key];
            if (mapping.type === "boolean")
                value = value === "true";
        }

        
        internals[property] = value;

        
        i++;
    }

    
    
    var s = GetOption(options, "sensitivity", "string",
                      ["base", "accent", "case", "variant"], undefined);
    if (s === undefined) {
        if (u === "sort") {
            
            s = "variant";
        } else {
            
            var dataLocale = r.dataLocale;
            var dataLocaleData = localeData(dataLocale);
            s = dataLocaleData.sensitivity;
        }
    }

    
    internals.sensitivity = s;

    
    var ip = GetOption(options, "ignorePunctuation", "boolean", undefined, false);
    internals.ignorePunctuation = ip;

    
    internals.boundFormat = undefined;

    
    internals.initializedCollator = true;
}









function Intl_Collator_supportedLocalesOf(locales ) {
    var options = arguments.length > 1 ? arguments[1] : undefined;

    var availableLocales = collatorInternalProperties.availableLocales;
    var requestedLocales = CanonicalizeLocaleList(locales);
    return SupportedLocales(availableLocales, requestedLocales, options);
}







var collatorInternalProperties = {
    sortLocaleData: collatorSortLocaleData,
    searchLocaleData: collatorSearchLocaleData,
    availableLocales: runtimeAvailableLocales, 
    relevantExtensionKeys: ["co", "kn"]
};


function collatorSortLocaleData(locale) {
    
    return {
        co: [null],
        kn: ["false", "true"]
    };
}


function collatorSearchLocaleData(locale) {
    
    return {
        co: [null],
        kn: ["false", "true"],
        sensitivity: "variant"
    };
}







function collatorCompareToBind(x, y) {
    
    

    
    var X = ToString(x);
    var Y = ToString(y);
    return CompareStrings(this, X, Y);
}











function Intl_Collator_compare_get() {
    
    var internals = checkIntlAPIObject(this, "Collator", "compare");

    
    if (internals.boundCompare === undefined) {
        
        var F = collatorCompareToBind;

        
        var bc = callFunction(std_Function_bind, F, this);
        internals.boundCompare = bc;
    }

    
    return internals.boundCompare;
}










function CompareStrings(collator, x, y) {
    assert(typeof x === "string", "CompareStrings");
    assert(typeof y === "string", "CompareStrings");

    
    return x.localeCompare(y);
}







function Intl_Collator_resolvedOptions() {
    
    var internals = checkIntlAPIObject(this, "Collator", "resolvedOptions");

    var result = {
        locale: internals.locale,
        usage: internals.usage,
        sensitivity: internals.sensitivity,
        ignorePunctuation: internals.ignorePunctuation
    };

    var relevantExtensionKeys = collatorInternalProperties.relevantExtensionKeys;
    for (var i = 0; i < relevantExtensionKeys.length; i++) {
        var key = relevantExtensionKeys[i];
        var property = (key === "co") ? "collation" : collatorKeyMappings[key].property;
        defineProperty(result, property, internals[property]);
    }
    return result;
}
