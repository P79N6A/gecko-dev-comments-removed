








$INCLUDE("testIntl.js");

taintProperties(["localeMatcher"]);

var locale = new Intl.NumberFormat(undefined, {localeMatcher: "lookup"}).resolvedOptions().locale;
if (!isCanonicalizedStructurallyValidLanguageTag(locale)) {
    $ERROR("NumberFormat returns invalid locale " + locale + ".");
}

