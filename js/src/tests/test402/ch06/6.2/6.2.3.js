








$INCLUDE("testIntl.js");

var canonicalizedTags = {
    "de": ["de"],
    "de-DE": ["de-DE", "de"],
    "DE-de": ["de-DE", "de"],
    "cmn": ["cmn"],
    "CMN-hANS": ["cmn-Hans", "cmn"],
    "cmn-hans-cn": ["cmn-Hans-CN", "cmn-Hans", "cmn"],
    "es-419": ["es-419", "es"],
    "es-419-u-nu-latn": ["es-419-u-nu-latn", "es-419", "es", "es-u-nu-latn"],
    
    "cmn-hans-cn-u-ca-t-ca-x-t-u": ["cmn-Hans-CN-t-ca-u-ca-x-t-u", "cmn-Hans-CN-t-ca-x-t-u", "cmn-Hans-CN-t-ca-x-t", "cmn-Hans-CN-t-ca", "cmn-Hans-CN", "cmn-Hans", "cmn"],
    "enochian-enochian": ["enochian-enochian", "enochian"],
    "de-gregory-u-ca-gregory": ["de-gregory-u-ca-gregory", "de-gregory", "de-u-ca-gregory", "de"],
    "no-nyn": ["nn"],
    "i-klingon": ["tlh"],
    "sgn-GR": ["gss"],
    "ji": ["yi"],
    "de-DD": ["de-DE", "de"],
    "zh-hak-CN": ["hak-CN", "hak"],
    "sgn-ils": ["ils"],
    "in": ["id"],
    "x-foo": ["x-foo"]
};


Object.getOwnPropertyNames(canonicalizedTags).forEach(function (tag) {
    canonicalizedTags[tag].forEach(function (canonicalTag) {
        if (!isCanonicalizedStructurallyValidLanguageTag(canonicalTag)) {
            $ERROR("Test data \"" + canonicalTag + "\" is not canonicalized and structurally valid language tag.");
        }
    });
});


testWithIntlConstructors(function (Constructor) {
    var defaultLocale = new Constructor().resolvedOptions().locale;
    Object.getOwnPropertyNames(canonicalizedTags).forEach(function (tag) {
        

        
        
        var object = new Constructor([tag], {localeMatcher: "lookup"});
        var locale = object.resolvedOptions().locale;
        if (canonicalizedTags[tag].indexOf(locale) === -1 && locale !== defaultLocale) {
            $ERROR("For " + tag + " got " + locale + "; expected one of " +
                canonicalizedTags[tag].join(", ") + ".");
        }
        
        
        var supported = Constructor.supportedLocalesOf([tag]);
        if (supported.length > 0 && supported[0] !== canonicalizedTags[tag][0]) {
            $ERROR("For " + tag + " got " + supported[0] + "; expected " +
                canonicalizedTags[tag][0] + ".");
        }            
    });
    return true;
});

