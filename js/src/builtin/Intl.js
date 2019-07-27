





































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







var internalIntlRegExps = std_Object_create(null);
internalIntlRegExps.unicodeLocaleExtensionSequenceRE = null;
internalIntlRegExps.languageTagRE = null;
internalIntlRegExps.duplicateVariantRE = null;
internalIntlRegExps.duplicateSingletonRE = null;
internalIntlRegExps.isWellFormedCurrencyCodeRE = null;
internalIntlRegExps.currencyDigitsRE = null;













function getUnicodeLocaleExtensionSequenceRE() {
    return internalIntlRegExps.unicodeLocaleExtensionSequenceRE ||
           (internalIntlRegExps.unicodeLocaleExtensionSequenceRE =
            regexp_construct_no_statics("-u(-[a-z0-9]{2,8})+"));
}





function removeUnicodeExtensions(locale) {
    
    
    var extensions;
    var unicodeLocaleExtensionSequenceRE = getUnicodeLocaleExtensionSequenceRE();
    while ((extensions = regexp_exec_no_statics(unicodeLocaleExtensionSequenceRE, locale)) !== null) {
        locale = callFunction(std_String_replace, locale, extensions[0], "");
        unicodeLocaleExtensionSequenceRE.lastIndex = 0;
    }
    return locale;
}







function getLanguageTagRE() {
    if (internalIntlRegExps.languageTagRE)
        return internalIntlRegExps.languageTagRE;

    
    
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

    
    return (internalIntlRegExps.languageTagRE =
            regexp_construct_no_statics(languageTag, "i"));
}


function getDuplicateVariantRE() {
    if (internalIntlRegExps.duplicateVariantRE)
        return internalIntlRegExps.duplicateVariantRE;

    
    
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

    
    
    
    return (internalIntlRegExps.duplicateVariantRE =
            regexp_construct_no_statics(duplicateVariant));
}


function getDuplicateSingletonRE() {
    if (internalIntlRegExps.duplicateSingletonRE)
        return internalIntlRegExps.duplicateSingletonRE;

    
    
    var ALPHA = "[a-zA-Z]";
    
    
    var DIGIT = "[0-9]";

    
    
    var alphanum = "(?:" + ALPHA + "|" + DIGIT + ")";
    
    
    
    
    
    var singleton = "(?:" + DIGIT + "|[A-WY-Za-wy-z])";

    
    var duplicateSingleton =
        
        
        "-(" + singleton + ")-" +
        
        "(?:" + alphanum + "+-)*" +
        
        "\\1" +
        
        
        "(?!" + alphanum + ")";

    
    
    
    return (internalIntlRegExps.duplicateSingletonRE =
            regexp_construct_no_statics(duplicateSingleton));
}








function IsStructurallyValidLanguageTag(locale) {
    assert(typeof locale === "string", "IsStructurallyValidLanguageTag");
    var languageTagRE = getLanguageTagRE();
    if (!regexp_test_no_statics(languageTagRE, locale))
        return false;

    
    
    
    if (callFunction(std_String_startsWith, locale, "x-"))
        return true;
    var pos = callFunction(std_String_indexOf, locale, "-x-");
    if (pos !== -1)
        locale = callFunction(std_String_substring, locale, 0, pos);

    
    var duplicateVariantRE = getDuplicateVariantRE();
    var duplicateSingletonRE = getDuplicateSingletonRE();
    return !regexp_test_no_statics(duplicateVariantRE, locale) &&
           !regexp_test_no_statics(duplicateSingletonRE, locale);
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

    if (!(collatorInternalProperties.availableLocales()[locale] &&
          numberFormatInternalProperties.availableLocales()[locale] &&
          dateTimeFormatInternalProperties.availableLocales()[locale]))
    {
        locale = localeOfLastResort;
    }
    return locale;
}







function getIsWellFormedCurrencyCodeRE() {
    return internalIntlRegExps.isWellFormedCurrencyCodeRE ||
           (internalIntlRegExps.isWellFormedCurrencyCodeRE =
            regexp_construct_no_statics("[^A-Z]"));
}
function IsWellFormedCurrencyCode(currency) {
    var c = ToString(currency);
    var normalized = toASCIIUpperCase(c);
    if (normalized.length !== 3)
        return false;
    return !regexp_test_no_statics(getIsWellFormedCurrencyCodeRE(), normalized);
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
                ThrowTypeError(JSMSG_INVALID_LOCALES_ELEMENT);
            var tag = ToString(kValue);
            if (!IsStructurallyValidLanguageTag(tag))
                ThrowRangeError(JSMSG_INVALID_LANGUAGE_TAG, tag);
            tag = CanonicalizeLanguageTag(tag);
            if (seen.indexOf(tag) === -1)
                seen.push(tag);
        }
        k++;
    }
    return seen;
}










function BestAvailableLocale(availableLocales, locale) {
    assert(IsStructurallyValidLanguageTag(locale), "invalid BestAvailableLocale locale structure");
    assert(locale === CanonicalizeLanguageTag(locale), "non-canonical BestAvailableLocale locale");
    assert(callFunction(std_String_indexOf, locale, "-u-") === -1, "locale shouldn't contain -u-");

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
        noExtensionsLocale = removeUnicodeExtensions(locale);
        availableLocale = BestAvailableLocale(availableLocales, noExtensionsLocale);
        i++;
    }

    var result = new Record();
    if (availableLocale !== undefined) {
        result.locale = availableLocale;
        if (locale !== noExtensionsLocale) {
            var unicodeLocaleExtensionSequenceRE = getUnicodeLocaleExtensionSequenceRE();
            var extensionMatch = regexp_exec_no_statics(unicodeLocaleExtensionSequenceRE, locale);
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
        var noExtensionsLocale = removeUnicodeExtensions(locale);

        
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
                ThrowRangeError(JSMSG_INVALID_LOCALE_MATCHER, matcher);
        }
    }

    
    var subset = (matcher === undefined || matcher === "best fit")
                 ? BestFitSupportedLocales(availableLocales, requestedLocales)
                 : LookupSupportedLocales(availableLocales, requestedLocales);

    
    for (var i = 0; i < subset.length; i++) {
        _DefineDataProperty(subset, i, subset[i],
                            ATTR_ENUMERABLE | ATTR_NONCONFIGURABLE | ATTR_NONWRITABLE);
    }
    _DefineDataProperty(subset, "length", subset.length,
                        ATTR_NONENUMERABLE | ATTR_NONCONFIGURABLE | ATTR_NONWRITABLE);

    
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
            ThrowRangeError(JSMSG_INVALID_OPTION_VALUE, property, value);

        
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
        if (Number_isNaN(value) || value < minimum || value > maximum)
            ThrowRangeError(JSMSG_INVALID_DIGITS_VALUE, value);
        return std_Math_floor(value);
    }

    
    return fallback;
}









function defineProperty(o, p, v) {
    _DefineDataProperty(o, p, v, ATTR_ENUMERABLE | ATTR_CONFIGURABLE | ATTR_WRITABLE);
}













var internalsMap = new WeakMap();





function initializeIntlObject(obj) {
    assert(IsObject(obj), "Non-object passed to initializeIntlObject");

    
    

    var internals = std_Object_create(null);

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    internals.type = "partial";
    internals.lazyData = null;
    internals.internalProps = null;

    callFunction(std_WeakMap_set, internalsMap, obj, internals);
    return internals;
}





function setLazyData(internals, type, lazyData)
{
    assert(internals.type === "partial", "can't set lazy data for anything but a newborn");
    assert(type === "Collator" || type === "DateTimeFormat" || type == "NumberFormat", "bad type");
    assert(IsObject(lazyData), "non-object lazy data");

    
    internals.lazyData = lazyData;
    internals.type = type;
}






function setInternalProperties(internals, internalProps)
{
    assert(internals.type !== "partial", "newborn internals can't have computed internals");
    assert(IsObject(internals.lazyData), "lazy data must exist already");
    assert(IsObject(internalProps), "internalProps argument should be an object");

    
    internals.internalProps = internalProps;
    internals.lazyData = null;
}






function maybeInternalProperties(internals)
{
    assert(IsObject(internals), "non-object passed to maybeInternalProperties");
    assert(internals.type !== "partial", "maybeInternalProperties must only be used on completely-initialized internals objects");
    var lazyData = internals.lazyData;
    if (lazyData)
        return null;
    assert(IsObject(internals.internalProps), "missing lazy data and computed internals");
    return internals.internalProps;
}





function isInitializedIntlObject(obj) {
#ifdef DEBUG
    var internals = callFunction(std_WeakMap_get, internalsMap, obj);
    if (IsObject(internals)) {
        assert(callFunction(std_Object_hasOwnProperty, internals, "type"), "missing type");
        var type = internals.type;
        assert(type === "partial" || type === "Collator" || type === "DateTimeFormat" || type === "NumberFormat", "unexpected type");
        assert(callFunction(std_Object_hasOwnProperty, internals, "lazyData"), "missing lazyData");
        assert(callFunction(std_Object_hasOwnProperty, internals, "internalProps"), "missing internalProps");
    } else {
        assert(internals === undefined, "bad mapping for |obj|");
    }
#endif
    return callFunction(std_WeakMap_has, internalsMap, obj);
}














function getIntlObjectInternals(obj, className, methodName) {
    assert(typeof className === "string", "bad className for getIntlObjectInternals");

    var internals = callFunction(std_WeakMap_get, internalsMap, obj);
    assert(internals === undefined || isInitializedIntlObject(obj), "bad mapping in internalsMap");

    if (internals === undefined || internals.type !== className)
        ThrowTypeError(JSMSG_INTL_OBJECT_NOT_INITED, className, methodName, className);

    return internals;
}






function getInternals(obj)
{
    assert(isInitializedIntlObject(obj), "for use only on guaranteed Intl objects");

    var internals = callFunction(std_WeakMap_get, internalsMap, obj);

    assert(internals.type !== "partial", "must have been successfully initialized");
    var lazyData = internals.lazyData;
    if (!lazyData)
        return internals.internalProps;

    var internalProps;
    var type = internals.type;
    if (type === "Collator")
        internalProps = resolveCollatorInternals(lazyData)
    else if (type === "DateTimeFormat")
        internalProps = resolveDateTimeFormatInternals(lazyData)
    else
        internalProps = resolveNumberFormatInternals(lazyData);
    setInternalProperties(internals, internalProps);
    return internalProps;
}











var collatorKeyMappings = {
    kn: {property: "numeric", type: "boolean"},
    kf: {property: "caseFirst", type: "string", values: ["upper", "lower", "false"]}
};





function resolveCollatorInternals(lazyCollatorData)
{
    assert(IsObject(lazyCollatorData), "lazy data not an object?");

    var internalProps = std_Object_create(null);

    
    internalProps.usage = lazyCollatorData.usage;

    
    var Collator = collatorInternalProperties;

    
    var collatorIsSorting = lazyCollatorData.usage === "sort";
    var localeData = collatorIsSorting
                     ? Collator.sortLocaleData
                     : Collator.searchLocaleData;

    
    
    var relevantExtensionKeys = Collator.relevantExtensionKeys;

    
    var r = ResolveLocale(Collator.availableLocales(),
                          lazyCollatorData.requestedLocales,
                          lazyCollatorData.opt,
                          relevantExtensionKeys,
                          localeData);

    
    internalProps.locale = r.locale;

    
    var key, property, value, mapping;
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

        
        internalProps[property] = value;

        
        i++;
    }

    
    
    var s = lazyCollatorData.rawSensitivity;
    if (s === undefined) {
        if (collatorIsSorting) {
            
            s = "variant";
        } else {
            
            var dataLocale = r.dataLocale;
            var dataLocaleData = localeData(dataLocale);
            s = dataLocaleData.sensitivity;
        }
    }
    internalProps.sensitivity = s;

    
    internalProps.ignorePunctuation = lazyCollatorData.ignorePunctuation;

    
    internalProps.boundFormat = undefined;

    
    
    return internalProps;
}






function getCollatorInternals(obj, methodName) {
    var internals = getIntlObjectInternals(obj, "Collator", methodName);
    assert(internals.type === "Collator", "bad type escaped getIntlObjectInternals");

    
    var internalProps = maybeInternalProperties(internals);
    if (internalProps)
        return internalProps;

    
    internalProps = resolveCollatorInternals(internals.lazyData);
    setInternalProperties(internals, internalProps);
    return internalProps;
}













function InitializeCollator(collator, locales, options) {
    assert(IsObject(collator), "InitializeCollator");

    
    if (isInitializedIntlObject(collator))
        ThrowTypeError(JSMSG_INTL_OBJECT_REINITED);

    
    var internals = initializeIntlObject(collator);

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    var lazyCollatorData = std_Object_create(null);

    
    var requestedLocales = CanonicalizeLocaleList(locales);
    lazyCollatorData.requestedLocales = requestedLocales;

    
    
    
    
    
    
    
    if (options === undefined)
        options = {};
    else
        options = ToObject(options);

    
    
    var u = GetOption(options, "usage", "string", ["sort", "search"], "sort");
    lazyCollatorData.usage = u;

    
    var opt = new Record();
    lazyCollatorData.opt = opt;

    
    var matcher = GetOption(options, "localeMatcher", "string", ["lookup", "best fit"], "best fit");
    opt.localeMatcher = matcher;

    
    var numericValue = GetOption(options, "numeric", "boolean", undefined, undefined);
    if (numericValue !== undefined)
        numericValue = numericValue ? 'true' : 'false';
    opt.kn = numericValue;

    var caseFirstValue = GetOption(options, "caseFirst", "string", ["upper", "lower", "false"], undefined);
    opt.kf = caseFirstValue;

    
    
    var s = GetOption(options, "sensitivity", "string",
                      ["base", "accent", "case", "variant"], undefined);
    lazyCollatorData.rawSensitivity = s;

    
    var ip = GetOption(options, "ignorePunctuation", "boolean", undefined, false);
    lazyCollatorData.ignorePunctuation = ip;

    
    
    
    
    setLazyData(internals, "Collator", lazyCollatorData);
}









function Intl_Collator_supportedLocalesOf(locales ) {
    var options = arguments.length > 1 ? arguments[1] : undefined;

    var availableLocales = collatorInternalProperties.availableLocales();
    var requestedLocales = CanonicalizeLocaleList(locales);
    return SupportedLocales(availableLocales, requestedLocales, options);
}







var collatorInternalProperties = {
    sortLocaleData: collatorSortLocaleData,
    searchLocaleData: collatorSearchLocaleData,
    _availableLocales: null,
    availableLocales: function()
    {
        var locales = this._availableLocales;
        if (locales)
            return locales;
        return (this._availableLocales =
          addOldStyleLanguageTags(intl_Collator_availableLocales()));
    },
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
    
    var internals = getCollatorInternals(this, "compare");

    
    if (internals.boundCompare === undefined) {
        
        var F = collatorCompareToBind;

        
        var bc = callFunction(std_Function_bind, F, this);
        internals.boundCompare = bc;
    }

    
    return internals.boundCompare;
}







function Intl_Collator_resolvedOptions() {
    
    var internals = getCollatorInternals(this, "resolvedOptions");

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










var numberFormatInternalProperties = {
    localeData: numberFormatLocaleData,
    _availableLocales: null,
    availableLocales: function()
    {
        var locales = this._availableLocales;
        if (locales)
            return locales;
        return (this._availableLocales =
          addOldStyleLanguageTags(intl_NumberFormat_availableLocales()));
    },
    relevantExtensionKeys: ["nu"]
};





function resolveNumberFormatInternals(lazyNumberFormatData) {
    assert(IsObject(lazyNumberFormatData), "lazy data not an object?");

    var internalProps = std_Object_create(null);

    
    var requestedLocales = lazyNumberFormatData.requestedLocales;

    
    
    var opt = lazyNumberFormatData.opt;

    
    
    var NumberFormat = numberFormatInternalProperties;

    
    var localeData = NumberFormat.localeData;

    
    var r = ResolveLocale(NumberFormat.availableLocales(),
                          lazyNumberFormatData.requestedLocales,
                          lazyNumberFormatData.opt,
                          NumberFormat.relevantExtensionKeys,
                          localeData);

    
    internalProps.locale = r.locale;
    internalProps.numberingSystem = r.nu;

    
    
    var s = lazyNumberFormatData.style;
    internalProps.style = s;

    
    if (s === "currency") {
        internalProps.currency = lazyNumberFormatData.currency;
        internalProps.currencyDisplay = lazyNumberFormatData.currencyDisplay;
    }

    
    internalProps.minimumIntegerDigits = lazyNumberFormatData.minimumIntegerDigits;

    
    internalProps.minimumFractionDigits = lazyNumberFormatData.minimumFractionDigits;

    
    internalProps.maximumFractionDigits = lazyNumberFormatData.maximumFractionDigits;

    
    if ("minimumSignificantDigits" in lazyNumberFormatData) {
        
        
        assert("maximumSignificantDigits" in lazyNumberFormatData, "min/max sig digits mismatch");
        internalProps.minimumSignificantDigits = lazyNumberFormatData.minimumSignificantDigits;
        internalProps.maximumSignificantDigits = lazyNumberFormatData.maximumSignificantDigits;
    }

    
    internalProps.useGrouping = lazyNumberFormatData.useGrouping;

    
    internalProps.boundFormat = undefined;

    
    
    return internalProps;
}






function getNumberFormatInternals(obj, methodName) {
    var internals = getIntlObjectInternals(obj, "NumberFormat", methodName);
    assert(internals.type === "NumberFormat", "bad type escaped getIntlObjectInternals");

    
    var internalProps = maybeInternalProperties(internals);
    if (internalProps)
        return internalProps;

    
    internalProps = resolveNumberFormatInternals(internals.lazyData);
    setInternalProperties(internals, internalProps);
    return internalProps;
}













function InitializeNumberFormat(numberFormat, locales, options) {
    assert(IsObject(numberFormat), "InitializeNumberFormat");

    
    if (isInitializedIntlObject(numberFormat))
        ThrowTypeError(JSMSG_INTL_OBJECT_REINITED);

    
    var internals = initializeIntlObject(numberFormat);

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    var lazyNumberFormatData = std_Object_create(null);

    
    var requestedLocales = CanonicalizeLocaleList(locales);
    lazyNumberFormatData.requestedLocales = requestedLocales;

    
    
    
    
    
    
    
    if (options === undefined)
        options = {};
    else
        options = ToObject(options);

    
    
    var opt = new Record();
    lazyNumberFormatData.opt = opt;

    
    var matcher = GetOption(options, "localeMatcher", "string", ["lookup", "best fit"], "best fit");
    opt.localeMatcher = matcher;

    
    
    var s = GetOption(options, "style", "string", ["decimal", "percent", "currency"], "decimal");
    lazyNumberFormatData.style = s;

    
    var c = GetOption(options, "currency", "string", undefined, undefined);
    if (c !== undefined && !IsWellFormedCurrencyCode(c))
        ThrowRangeError(JSMSG_INVALID_CURRENCY_CODE, c);
    var cDigits;
    if (s === "currency") {
        if (c === undefined)
            ThrowTypeError(JSMSG_UNDEFINED_CURRENCY);

        
        c = toASCIIUpperCase(c);
        lazyNumberFormatData.currency = c;
        cDigits = CurrencyDigits(c);
    }

    
    var cd = GetOption(options, "currencyDisplay", "string", ["code", "symbol", "name"], "symbol");
    if (s === "currency")
        lazyNumberFormatData.currencyDisplay = cd;

    
    var mnid = GetNumberOption(options, "minimumIntegerDigits", 1, 21, 1);
    lazyNumberFormatData.minimumIntegerDigits = mnid;

    
    var mnfdDefault = (s === "currency") ? cDigits : 0;
    var mnfd = GetNumberOption(options, "minimumFractionDigits", 0, 20, mnfdDefault);
    lazyNumberFormatData.minimumFractionDigits = mnfd;

    
    var mxfdDefault;
    if (s === "currency")
        mxfdDefault = std_Math_max(mnfd, cDigits);
    else if (s === "percent")
        mxfdDefault = std_Math_max(mnfd, 0);
    else
        mxfdDefault = std_Math_max(mnfd, 3);
    var mxfd = GetNumberOption(options, "maximumFractionDigits", mnfd, 20, mxfdDefault);
    lazyNumberFormatData.maximumFractionDigits = mxfd;

    
    var mnsd = options.minimumSignificantDigits;
    var mxsd = options.maximumSignificantDigits;

    
    if (mnsd !== undefined || mxsd !== undefined) {
        mnsd = GetNumberOption(options, "minimumSignificantDigits", 1, 21, 1);
        mxsd = GetNumberOption(options, "maximumSignificantDigits", mnsd, 21, 21);
        lazyNumberFormatData.minimumSignificantDigits = mnsd;
        lazyNumberFormatData.maximumSignificantDigits = mxsd;
    }

    
    var g = GetOption(options, "useGrouping", "boolean", undefined, true);
    lazyNumberFormatData.useGrouping = g;

    
    
    
    
    setLazyData(internals, "NumberFormat", lazyNumberFormatData);
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
    UGX: 0,
    UYI: 0,
    VND: 0,
    VUV: 0,
    XAF: 0,
    XOF: 0,
    XPF: 0
};







function getCurrencyDigitsRE() {
    return internalIntlRegExps.currencyDigitsRE ||
           (internalIntlRegExps.currencyDigitsRE =
            regexp_construct_no_statics("^[A-Z]{3}$"));
}
function CurrencyDigits(currency) {
    assert(typeof currency === "string", "CurrencyDigits");
    assert(regexp_test_no_statics(getCurrencyDigitsRE(), currency), "CurrencyDigits");

    if (callFunction(std_Object_hasOwnProperty, currencyDigits, currency))
        return currencyDigits[currency];
    return 2;
}









function Intl_NumberFormat_supportedLocalesOf(locales ) {
    var options = arguments.length > 1 ? arguments[1] : undefined;

    var availableLocales = numberFormatInternalProperties.availableLocales();
    var requestedLocales = CanonicalizeLocaleList(locales);
    return SupportedLocales(availableLocales, requestedLocales, options);
}


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
    
    var internals = getNumberFormatInternals(this, "format");

    
    if (internals.boundFormat === undefined) {
        
        var F = numberFormatFormatToBind;

        
        var bf = callFunction(std_Function_bind, F, this);
        internals.boundFormat = bf;
    }
    
    return internals.boundFormat;
}







function Intl_NumberFormat_resolvedOptions() {
    
    var internals = getNumberFormatInternals(this, "resolvedOptions");

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








function resolveDateTimeFormatInternals(lazyDateTimeFormatData) {
    assert(IsObject(lazyDateTimeFormatData), "lazy data not an object?");

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    var internalProps = std_Object_create(null);

    
    
    var DateTimeFormat = dateTimeFormatInternalProperties;

    
    var localeData = DateTimeFormat.localeData;

    
    var r = ResolveLocale(DateTimeFormat.availableLocales(),
                          lazyDateTimeFormatData.requestedLocales,
                          lazyDateTimeFormatData.localeOpt,
                          DateTimeFormat.relevantExtensionKeys,
                          localeData);

    
    internalProps.locale = r.locale;
    internalProps.calendar = r.ca;
    internalProps.numberingSystem = r.nu;

    
    
    var dataLocale = r.dataLocale;

    
    internalProps.timeZone = lazyDateTimeFormatData.timeZone;

    
    var formatOpt = lazyDateTimeFormatData.formatOpt;

    
    var pattern = toBestICUPattern(dataLocale, formatOpt);

    
    internalProps.pattern = pattern;

    
    internalProps.boundFormat = undefined;

    
    
    return internalProps;
}






function getDateTimeFormatInternals(obj, methodName) {
    var internals = getIntlObjectInternals(obj, "DateTimeFormat", methodName);
    assert(internals.type === "DateTimeFormat", "bad type escaped getIntlObjectInternals");

    
    var internalProps = maybeInternalProperties(internals);
    if (internalProps)
        return internalProps;

    
    internalProps = resolveDateTimeFormatInternals(internals.lazyData);
    setInternalProperties(internals, internalProps);
    return internalProps;
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
        ThrowTypeError(JSMSG_INTL_OBJECT_REINITED);

    
    var internals = initializeIntlObject(dateTimeFormat);

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    var lazyDateTimeFormatData = std_Object_create(null);

    
    var requestedLocales = CanonicalizeLocaleList(locales);
    lazyDateTimeFormatData.requestedLocales = requestedLocales;

    
    options = ToDateTimeOptions(options, "any", "date");

    
    
    var localeOpt = new Record();
    lazyDateTimeFormatData.localeOpt = localeOpt;

    
    var localeMatcher =
        GetOption(options, "localeMatcher", "string", ["lookup", "best fit"],
                  "best fit");
    localeOpt.localeMatcher = localeMatcher;

    
    var tz = options.timeZone;
    if (tz !== undefined) {
        tz = toASCIIUpperCase(ToString(tz));
        if (tz !== "UTC")
            ThrowRangeError(JSMSG_INVALID_TIME_ZONE, tz);
    }
    lazyDateTimeFormatData.timeZone = tz;

    
    var formatOpt = new Record();
    lazyDateTimeFormatData.formatOpt = formatOpt;

    
    var i, prop;
    for (i = 0; i < dateTimeComponents.length; i++) {
        prop = dateTimeComponents[i];
        var value = GetOption(options, prop, "string", dateTimeComponentValues[prop], undefined);
        formatOpt[prop] = value;
    }

    

    
    
    
    
    
    
    var formatMatcher =
        GetOption(options, "formatMatcher", "string", ["basic", "best fit"],
                  "best fit");

    

    
    var hr12  = GetOption(options, "hour12", "boolean", undefined, undefined);

    
    if (hr12 !== undefined)
        formatOpt.hour12 = hr12;

    
    
    
    
    setLazyData(internals, "DateTimeFormat", lazyDateTimeFormatData);
}











































































function toBestICUPattern(locale, options) {
    
    
    var skeleton = "";
    switch (options.weekday) {
    case "narrow":
        skeleton += "EEEEE";
        break;
    case "short":
        skeleton += "E";
        break;
    case "long":
        skeleton += "EEEE";
    }
    switch (options.era) {
    case "narrow":
        skeleton += "GGGGG";
        break;
    case "short":
        skeleton += "G";
        break;
    case "long":
        skeleton += "GGGG";
        break;
    }
    switch (options.year) {
    case "2-digit":
        skeleton += "yy";
        break;
    case "numeric":
        skeleton += "y";
        break;
    }
    switch (options.month) {
    case "2-digit":
        skeleton += "MM";
        break;
    case "numeric":
        skeleton += "M";
        break;
    case "narrow":
        skeleton += "MMMMM";
        break;
    case "short":
        skeleton += "MMM";
        break;
    case "long":
        skeleton += "MMMM";
        break;
    }
    switch (options.day) {
    case "2-digit":
        skeleton += "dd";
        break;
    case "numeric":
        skeleton += "d";
        break;
    }
    var hourSkeletonChar = "j";
    if (options.hour12 !== undefined) {
        if (options.hour12)
            hourSkeletonChar = "h";
        else
            hourSkeletonChar = "H";
    }
    switch (options.hour) {
    case "2-digit":
        skeleton += hourSkeletonChar + hourSkeletonChar;
        break;
    case "numeric":
        skeleton += hourSkeletonChar;
        break;
    }
    switch (options.minute) {
    case "2-digit":
        skeleton += "mm";
        break;
    case "numeric":
        skeleton += "m";
        break;
    }
    switch (options.second) {
    case "2-digit":
        skeleton += "ss";
        break;
    case "numeric":
        skeleton += "s";
        break;
    }
    switch (options.timeZoneName) {
    case "short":
        skeleton += "z";
        break;
    case "long":
        skeleton += "zzzz";
        break;
    }

    
    return intl_patternForSkeleton(locale, skeleton);
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

    var availableLocales = dateTimeFormatInternalProperties.availableLocales();
    var requestedLocales = CanonicalizeLocaleList(locales);
    return SupportedLocales(availableLocales, requestedLocales, options);
}







var dateTimeFormatInternalProperties = {
    localeData: dateTimeFormatLocaleData,
    _availableLocales: null,
    availableLocales: function()
    {
        var locales = this._availableLocales;
        if (locales)
            return locales;
        return (this._availableLocales =
          addOldStyleLanguageTags(intl_DateTimeFormat_availableLocales()));
    },
    relevantExtensionKeys: ["ca", "nu"]
};


function dateTimeFormatLocaleData(locale) {
    return {
        ca: intl_availableCalendars(locale),
        nu: getNumberingSystems(locale)
    };
}







function dateTimeFormatFormatToBind() {
    
    var date = arguments.length > 0 ? arguments[0] : undefined;
    var x = (date === undefined) ? std_Date_now() : ToNumber(date);

    
    return intl_FormatDateTime(this, x);
}









function Intl_DateTimeFormat_format_get() {
    
    var internals = getDateTimeFormatInternals(this, "format");

    
    if (internals.boundFormat === undefined) {
        
        var F = dateTimeFormatFormatToBind;

        
        var bf = callFunction(std_Function_bind, F, this);
        internals.boundFormat = bf;
    }

    
    return internals.boundFormat;
}







function Intl_DateTimeFormat_resolvedOptions() {
    
    var internals = getDateTimeFormatInternals(this, "resolvedOptions");

    var result = {
        locale: internals.locale,
        calendar: internals.calendar,
        numberingSystem: internals.numberingSystem,
        timeZone: internals.timeZone
    };
    resolveICUPattern(internals.pattern, result);
    return result;
}





var icuPatternCharToComponent = {
    E: "weekday",
    G: "era",
    y: "year",
    M: "month",
    L: "month",
    d: "day",
    h: "hour",
    H: "hour",
    k: "hour",
    K: "hour",
    m: "minute",
    s: "second",
    z: "timeZoneName",
    v: "timeZoneName",
    V: "timeZoneName"
};









function resolveICUPattern(pattern, result) {
    assert(IsObject(result), "resolveICUPattern");
    var i = 0;
    while (i < pattern.length) {
        var c = pattern[i++];
        if (c === "'") {
            while (i < pattern.length && pattern[i] !== "'")
                i++;
            i++;
        } else {
            var count = 1;
            while (i < pattern.length && pattern[i] === c) {
                i++;
                count++;
            }
            var value;
            switch (c) {
            
            case "G":
            case "E":
            case "z":
            case "v":
            case "V":
                if (count <= 3)
                    value = "short";
                else if (count === 4)
                    value = "long";
                else
                    value = "narrow";
                break;
            
            case "y":
            case "d":
            case "h":
            case "H":
            case "m":
            case "s":
            case "k":
            case "K":
                if (count === 2)
                    value = "2-digit";
                else
                    value = "numeric";
                break;
            
            case "M":
            case "L":
                if (count === 1)
                    value = "numeric";
                else if (count === 2)
                    value = "2-digit";
                else if (count === 3)
                    value = "short";
                else if (count === 4)
                    value = "long";
                else
                    value = "narrow";
                break;
            default:
                
            }
            if (callFunction(std_Object_hasOwnProperty, icuPatternCharToComponent, c))
                defineProperty(result, icuPatternCharToComponent[c], value);
            if (c === "h" || c === "K")
                defineProperty(result, "hour12", true);
            else if (c === "H" || c === "k")
                defineProperty(result, "hour12", false);
        }
    }
}
