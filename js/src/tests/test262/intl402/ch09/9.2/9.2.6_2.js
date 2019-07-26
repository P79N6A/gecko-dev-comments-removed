








$INCLUDE("testIntl.js");

taintArray();

testWithIntlConstructors(function (Constructor) {
    
    ["lookup", "best fit"].forEach(function (matcher) {
        var defaultLocale = new Constructor().resolvedOptions().locale;
        var canonicalized = Constructor.supportedLocalesOf([defaultLocale, defaultLocale],
            {localeMatcher: matcher});
        if (canonicalized.length > 1) {
            $ERROR("Canonicalization with matcher " + matcher + " didn't remove duplicate language tags from locale list.");
        }
    });

    return true;
});

