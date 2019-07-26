








$INCLUDE("testIntl.js");

taintProperties(["locale", "extension", "extensionIndex"]);

testWithIntlConstructors(function (Constructor) {
    var locale = new Constructor(undefined, {localeMatcher: "lookup"}).resolvedOptions().locale;
    if (!isCanonicalizedStructurallyValidLanguageTag(locale)) {
        $ERROR("Constructor returns invalid locale " + locale + ".");
    }

    return true;
});

