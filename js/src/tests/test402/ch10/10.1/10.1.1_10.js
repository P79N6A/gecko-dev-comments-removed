








$INCLUDE("testIntl.js");

taintProperties(["localeMatcher", "kn", "kf"]);

var locale = new Intl.Collator(undefined, {localeMatcher: "lookup"}).resolvedOptions().locale;
if (!isCanonicalizedStructurallyValidLanguageTag(locale)) {
    $ERROR("Collator returns invalid locale " + locale + ".");
}

