





































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
    
    
    
    
    
    var localeOfLastResort = "en-GB";

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
    availableLocales: addOldStyleLanguageTags(intl_Collator_availableLocales()),
    relevantExtensionKeys: ["co", "kn"]
};


function collatorSortLocaleData(locale) {
    var collations = intl_availableCollations(locale);
    callFunction(std_Array_unshift, collations, null);
    return {
        co: collations,
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
    return intl_CompareStrings(this, X, Y);
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










function InitializeNumberFormat(numberFormat, locales, options) {
    assert(IsObject(numberFormat), "InitializeNumberFormat");

    
    if (isInitializedIntlObject(numberFormat))
        ThrowError(JSMSG_INTL_OBJECT_REINITED);

    
    var internals = initializeIntlObject(numberFormat);

    
    var requestedLocales = CanonicalizeLocaleList(locales);

    
    if (options === undefined)
        options = {};
    else
        options = ToObject(options);

    
    
    var opt = new Record();

    
    var matcher = GetOption(options, "localeMatcher", "string", ["lookup", "best fit"], "best fit");
    opt.localeMatcher = matcher;

    
    
    var NumberFormat = numberFormatInternalProperties;

    
    var localeData = NumberFormat.localeData;

    
    var r = ResolveLocale(NumberFormat.availableLocales,
                          requestedLocales, opt,
                          NumberFormat.relevantExtensionKeys,
                          localeData);

    
    internals.locale = r.locale;
    internals.numberingSystem = r.nu;
    var dataLocale = r.dataLocale;

    
    
    var s = GetOption(options, "style", "string", ["decimal", "percent", "currency"], "decimal");
    internals.style = s;

    
    var c = GetOption(options, "currency", "string", undefined, undefined);
    if (c !== undefined && !IsWellFormedCurrencyCode(c))
        ThrowError(JSMSG_INVALID_CURRENCY_CODE, c);
    var cDigits;
    if (s === "currency") {
        if (c === undefined)
            ThrowError(JSMSG_UNDEFINED_CURRENCY);

        
        c = toASCIIUpperCase(c);
        internals.currency = c;
        cDigits = CurrencyDigits(c);
    }

    
    var cd = GetOption(options, "currencyDisplay", "string", ["code", "symbol", "name"], "symbol");
    if (s === "currency")
        internals.currencyDisplay = cd;

    
    var mnid = GetNumberOption(options, "minimumIntegerDigits", 1, 21, 1);
    internals.minimumIntegerDigits = mnid;

    
    var mnfdDefault = (s === "currency") ? cDigits : 0;
    var mnfd = GetNumberOption(options, "minimumFractionDigits", 0, 20, mnfdDefault);
    internals.minimumFractionDigits = mnfd;

    
    var mxfdDefault;
    if (s === "currency")
        mxfdDefault = std_Math_max(mnfd, cDigits);
    else if (s === "percent")
        mxfdDefault = std_Math_max(mnfd, 0);
    else
        mxfdDefault = std_Math_max(mnfd, 3);
    var mxfd = GetNumberOption(options, "maximumFractionDigits", mnfd, 20, mxfdDefault);
    internals.maximumFractionDigits = mxfd;

    
    var mnsd = options.minimumSignificantDigits;
    var mxsd = options.maximumSignificantDigits;

    
    if (mnsd !== undefined || mxsd !== undefined) {
        mnsd = GetNumberOption(options, "minimumSignificantDigits", 1, 21, 1);
        mxsd = GetNumberOption(options, "maximumSignificantDigits", mnsd, 21, 21);
        internals.minimumSignificantDigits = mnsd;
        internals.maximumSignificantDigits = mxsd;
    }

    
    var g = GetOption(options, "useGrouping", "boolean", undefined, true);
    internals.useGrouping = g;

    

    
    internals.boundFormat = undefined;

    
    internals.initializedNumberFormat = true;
}









var currencyDigits = {
    BHD: 3,
    BIF: 0,
    BYR: 0,
    CLF: 0,
    CLP: 0,
    DJF: 0,
    IQD: 3,
    GNF: 0,
    ISK: 0,
    JOD: 3,
    JPY: 0,
    KMF: 0,
    KRW: 0,
    KWD: 3,
    LYD: 3,
    OMR: 3,
    PYG: 0,
    RWF: 0,
    TND: 3,
    UYI: 0,
    VND: 0,
    VUV: 0,
    XAF: 0,
    XOF: 0,
    XPF: 0
};







function CurrencyDigits(currency) {
    assert(typeof currency === "string", "CurrencyDigits");
    assert(callFunction(std_RegExp_test, /^[A-Z]{3}$/, currency), "CurrencyDigits");

    if (callFunction(std_Object_hasOwnProperty, currencyDigits, currency))
        return currencyDigits[currency];
    return 2;
}









function Intl_NumberFormat_supportedLocalesOf(locales ) {
    var options = arguments.length > 1 ? arguments[1] : undefined;

    var availableLocales = numberFormatInternalProperties.availableLocales;
    var requestedLocales = CanonicalizeLocaleList(locales);
    return SupportedLocales(availableLocales, requestedLocales, options);
}







var numberFormatInternalProperties = {
    localeData: numberFormatLocaleData,
    availableLocales: addOldStyleLanguageTags(intl_NumberFormat_availableLocales()),
    relevantExtensionKeys: ["nu"]
};


function getNumberingSystems(locale) {
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    var defaultNumberingSystem = intl_numberingSystem(locale);
    return [
        defaultNumberingSystem,
        "arab", "arabext", "bali", "beng", "deva",
        "fullwide", "gujr", "guru", "hanidec", "khmr",
        "knda", "laoo", "latn", "limb", "mlym",
        "mong", "mymr", "orya", "tamldec", "telu",
        "thai", "tibt"
    ];
}


function numberFormatLocaleData(locale) {
    return {
        nu: getNumberingSystems(locale)
    };
}







function numberFormatFormatToBind(value) {
    
    

    
    var x = ToNumber(value);
    return intl_FormatNumber(this, x);
}









function Intl_NumberFormat_format_get() {
    
    var internals = checkIntlAPIObject(this, "NumberFormat", "format");

    
    if (internals.boundFormat === undefined) {
        
        var F = numberFormatFormatToBind;

        
        var bf = callFunction(std_Function_bind, F, this);
        internals.boundFormat = bf;
    }
    
    return internals.boundFormat;
}







function Intl_NumberFormat_resolvedOptions() {
    
    var internals = checkIntlAPIObject(this, "NumberFormat", "resolvedOptions");

    var result = {
        locale: internals.locale,
        numberingSystem: internals.numberingSystem,
        style: internals.style,
        minimumIntegerDigits: internals.minimumIntegerDigits,
        minimumFractionDigits: internals.minimumFractionDigits,
        maximumFractionDigits: internals.maximumFractionDigits,
        useGrouping: internals.useGrouping
    };
    var optionalProperties = [
        "currency",
        "currencyDisplay",
        "minimumSignificantDigits",
        "maximumSignificantDigits"
    ];
    for (var i = 0; i < optionalProperties.length; i++) {
        var p = optionalProperties[i];
        if (callFunction(std_Object_hasOwnProperty, internals, p))
            defineProperty(result, p, internals[p]);
    }
    return result;
}










var dateTimeComponentValues = {
    weekday: ["narrow", "short", "long"],
    era: ["narrow", "short", "long"],
    year: ["2-digit", "numeric"],
    month: ["2-digit", "numeric", "narrow", "short", "long"],
    day: ["2-digit", "numeric"],
    hour: ["2-digit", "numeric"],
    minute: ["2-digit", "numeric"],
    second: ["2-digit", "numeric"],
    timeZoneName: ["short", "long"]
};


var dateTimeComponents = std_Object_getOwnPropertyNames(dateTimeComponentValues);







function InitializeDateTimeFormat(dateTimeFormat, locales, options) {
    assert(IsObject(dateTimeFormat), "InitializeDateTimeFormat");

    
    if (isInitializedIntlObject(dateTimeFormat))
        ThrowError(JSMSG_INTL_OBJECT_REINITED);

    
    var internals = initializeIntlObject(dateTimeFormat);

    
    var requestedLocales = CanonicalizeLocaleList(locales);

    
    options = ToDateTimeOptions(options, "any", "date");

    
    
    var opt = new Record();

    
    var matcher = GetOption(options, "localeMatcher", "string", ["lookup", "best fit"], "best fit");
    opt.localeMatcher = matcher;

    
    
    var DateTimeFormat = dateTimeFormatInternalProperties;

    
    var localeData = DateTimeFormat.localeData;

    
    var r = ResolveLocale(DateTimeFormat.availableLocales,
                          requestedLocales, opt,
                          DateTimeFormat.relevantExtensionKeys,
                          localeData);

    
    internals.locale = r.locale;
    internals.calendar = r.ca;
    internals.numberingSystem = r.nu;

    
    
    var dataLocale = r.dataLocale;

    
    var tz = options.timeZone;
    if (tz !== undefined) {
        tz = toASCIIUpperCase(ToString(tz));
        if (tz !== "UTC")
            ThrowError(JSMSG_INVALID_TIME_ZONE, tz);
    }
    internals.timeZone = tz;

    
    opt = new Record();

    
    var i, prop;
    for (i = 0; i < dateTimeComponents.length; i++) {
        prop = dateTimeComponents[i];
        var value = GetOption(options, prop, "string", dateTimeComponentValues[prop], undefined);
        opt[prop] = value;
    }

    
    var dataLocaleData = localeData(dataLocale);

    
    var formats = dataLocaleData.formats;

    
    matcher = GetOption(options, "formatMatcher", "string", ["basic", "best fit"], "best fit");
    var bestFormat = (matcher === "basic")
                     ? BasicFormatMatcher(opt, formats)
                     : BestFitFormatMatcher(opt, formats);

    
    for (i = 0; i < dateTimeComponents.length; i++) {
        prop = dateTimeComponents[i];
        if (callFunction(std_Object_hasOwnProperty, bestFormat, prop)) {
            var p = bestFormat[prop];
            internals[prop] = p;
        }
    }

    
    var hr12  = GetOption(options, "hour12", "boolean", undefined, undefined);

    
    var pattern;
    if (callFunction(std_Object_hasOwnProperty, internals, "hour")) {
        
        if (hr12 === undefined)
            hr12 = dataLocaleData.hour12;
        assert(typeof hr12 === "boolean");
        internals.hour12 = hr12;

        if (hr12) {
            
            var hourNo0 = dataLocaleData.hourNo0;
            internals.hourNo0 = hourNo0;
            pattern = bestFormat.pattern12;
        } else {
            
            pattern = bestFormat.pattern;
        }
    } else {
        
        pattern = bestFormat.pattern;
    }

    
    internals.pattern = pattern;

    
    internals.boundFormat = undefined;

    
    internals.initializedDateTimeFormat = true;
}










function ToDateTimeOptions(options, required, defaults) {
    assert(typeof required === "string", "ToDateTimeOptions");
    assert(typeof defaults === "string", "ToDateTimeOptions");

    
    if (options === undefined)
        options = null;
    else
        options = ToObject(options);
    options = std_Object_create(options);

    
    var needDefaults = true;

    
    if ((required === "date" || required === "any") &&
        (options.weekday !== undefined || options.year !== undefined ||
         options.month !== undefined || options.day !== undefined))
    {
        needDefaults = false;
    }

    
    if ((required === "time" || required === "any") &&
        (options.hour !== undefined || options.minute !== undefined ||
         options.second !== undefined))
    {
        needDefaults = false;
    }

    
    if (needDefaults && (defaults === "date" || defaults === "all")) {
        
        
        
        
        defineProperty(options, "year", "numeric");
        defineProperty(options, "month", "numeric");
        defineProperty(options, "day", "numeric");
    }

    
    if (needDefaults && (defaults === "time" || defaults === "all")) {
        
        defineProperty(options, "hour", "numeric");
        defineProperty(options, "minute", "numeric");
        defineProperty(options, "second", "numeric");
    }

    
    return options;
}









function BasicFormatMatcher(options, formats) {
    
    var removalPenalty = 120,
        additionPenalty = 20,
        longLessPenalty = 8,
        longMorePenalty = 6,
        shortLessPenalty = 6,
        shortMorePenalty = 3;

    
    var properties = ["weekday", "era", "year", "month", "day",
        "hour", "minute", "second", "timeZoneName"];

    
    var values = ["2-digit", "numeric", "narrow", "short", "long"];

    
    var bestScore = -Infinity;
    var bestFormat;

    
    var i = 0;
    var len = formats.length;
    while (i < len) {
        
        var format = formats[i];
        var score = 0;

        
        var formatProp;
        for (var j = 0; j < properties.length; j++) {
            var property = properties[j];

            
            var optionsProp = options[property];
            
            
            formatProp = undefined;

            
            if (callFunction(std_Object_hasOwnProperty, format, property))
                formatProp = format[property];

            if (optionsProp === undefined && formatProp !== undefined) {
                
                score -= additionPenalty;
            } else if (optionsProp !== undefined && formatProp === undefined) {
                
                score -= removalPenalty;
            } else {
                
                var optionsPropIndex = callFunction(std_Array_indexOf, values, optionsProp);
                var formatPropIndex = callFunction(std_Array_indexOf, values, formatProp);
                var delta = std_Math_max(std_Math_min(formatPropIndex - optionsPropIndex, 2), -2);
                if (delta === 2)
                    score -= longMorePenalty;
                else if (delta === 1)
                    score -= shortMorePenalty;
                else if (delta === -1)
                    score -= shortLessPenalty;
                else if (delta === -2)
                    score -= longLessPenalty;
            }
        }

        
        if (score > bestScore) {
            bestScore = score;
            bestFormat = format;
        }

        
        i++;
    }

    
    return bestFormat;
}









function BestFitFormatMatcher(options, formats) {
    
    return BasicFormatMatcher(options, formats);
}









function Intl_DateTimeFormat_supportedLocalesOf(locales ) {
    var options = arguments.length > 1 ? arguments[1] : undefined;

    var availableLocales = dateTimeFormatInternalProperties.availableLocales;
    var requestedLocales = CanonicalizeLocaleList(locales);
    return SupportedLocales(availableLocales, requestedLocales, options);
}







var dateTimeFormatInternalProperties = {
    localeData: dateTimeFormatLocaleData,
    availableLocales: runtimeAvailableLocales, 
    relevantExtensionKeys: ["ca", "nu"]
};


function dateTimeFormatLocaleData(locale) {
    
    var localeData = {
        ca: ["gregory"],
        nu: ["latn"],
        hour12: false,
        hourNo0: false
    };

    var formatDate = {
        weekday: "short",
        year: "numeric",
        month: "short",
        day: "numeric"
    };
    var formatTime = {
        hour: "numeric",
        minute: "numeric",
        second: "numeric"
    };
    var formatFull = {
        weekday: "short",
        year: "numeric",
        month: "short",
        day: "numeric",
        hour: "numeric",
        minute: "numeric",
        second: "numeric"
    };
    localeData.formats = [formatDate, formatTime, formatFull];

    return localeData;
}







function dateTimeFormatFormatToBind() {
    
    var date = arguments.length > 0 ? arguments[0] : undefined;
    var x = (date === undefined) ? std_Date_now() : ToNumber(date);

    
    return FormatDateTime(this, x);
}









function Intl_DateTimeFormat_format_get() {
    
    var internals = checkIntlAPIObject(this, "DateTimeFormat", "format");

    
    if (internals.boundFormat === undefined) {
        
        var F = dateTimeFormatFormatToBind;

        
        var bf = callFunction(std_Function_bind, F, this);
        internals.boundFormat = bf;
    }

    
    return internals.boundFormat;
}









function FormatDateTime(dateTimeFormat, x) {
    
    if (!std_isFinite(x))
        ThrowError(JSMSG_DATE_NOT_FINITE);
    var X = new Std_Date(x);
    var internals = getInternals(dateTimeFormat);
    var wantDate = callFunction(std_Object_hasOwnProperty, internals, "weekday") ||
        callFunction(std_Object_hasOwnProperty, internals, "year") ||
        callFunction(std_Object_hasOwnProperty, internals, "month") ||
        callFunction(std_Object_hasOwnProperty, internals, "day");
    var wantTime = callFunction(std_Object_hasOwnProperty, internals, "hour") ||
        callFunction(std_Object_hasOwnProperty, internals, "minute") ||
        callFunction(std_Object_hasOwnProperty, internals, "second");
    if (wantDate) {
        if (wantTime)
            return X.toLocaleString();
        return X.toLocaleDateString();
    }
    return X.toLocaleTimeString();
}







function Intl_DateTimeFormat_resolvedOptions() {
    
    var internals = checkIntlAPIObject(this, "DateTimeFormat", "resolvedOptions");

    var result = {
        locale: internals.locale,
        calendar: internals.calendar,
        numberingSystem: internals.numberingSystem,
        timeZone: internals.timeZone
    };
    for (var i = 0; i < dateTimeComponents.length; i++) {
        var p = dateTimeComponents[i];
        if (callFunction(std_Object_hasOwnProperty, internals, p))
            defineProperty(result, p, internals[p]);
    }
    if (callFunction(std_Object_hasOwnProperty, internals, "hour12"))
        defineProperty(result, "hour12", internals.hour12);
    return result;
}
