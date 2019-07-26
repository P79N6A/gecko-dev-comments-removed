







$INCLUDE("testIntl.js");

testWithIntlConstructors(function (Constructor) {
    var defaultLocale = new Constructor().resolvedOptions().locale;
    var canonicalized = Constructor.supportedLocalesOf([defaultLocale, defaultLocale]);
    if (canonicalized.length > 1) {
        $ERROR("Canonicalization didn't remove duplicate language tags from locale list.");
    }
});

