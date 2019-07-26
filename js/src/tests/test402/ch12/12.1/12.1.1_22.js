








$INCLUDE("testIntl.js");

taintProperties(["weekday", "era", "year", "month", "day", "hour", "minute", "second", "timeZone"]);

var locale = new Intl.DateTimeFormat(undefined, {localeMatcher: "lookup"}).resolvedOptions().locale;
if (!isCanonicalizedStructurallyValidLanguageTag(locale)) {
    $ERROR("DateTimeFormat returns invalid locale " + locale + ".");
}

