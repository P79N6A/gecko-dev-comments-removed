








$INCLUDE("testIntl.js");

testWithIntlConstructors(function (Constructor) {
    var defaultLocale = new Constructor().resolvedOptions().locale;
    if (!isCanonicalizedStructurallyValidLanguageTag(defaultLocale)) {
        $ERROR("Default locale \"" + defaultLocale + "\" is not canonicalized and structurally valid language tag.");
    }
    return true;
});

