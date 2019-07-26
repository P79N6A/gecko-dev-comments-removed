








$INCLUDE("testIntl.js");

testWithIntlConstructors(function (Constructor) {
    
    ["lookup", "best fit"].forEach(function (matcher) {
        var supported = Constructor.supportedLocalesOf([], {localeMatcher: matcher});
        if (supported.length !== 0) {
            $ERROR("SupportedLocales with matcher " + matcher + " returned a non-empty list for an empty list.");
        }
    });

    return true;
});

