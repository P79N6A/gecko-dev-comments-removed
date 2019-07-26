








$INCLUDE("testIntl.js");

testWithIntlConstructors(function (Constructor) {
    
    ["lookup", "best fit"].forEach(function (matcher) {
        var defaultLocale = new Constructor().resolvedOptions().locale;
        var noLinguisticContent = "zxx";
        var supported = Constructor.supportedLocalesOf([defaultLocale, noLinguisticContent],
            {localeMatcher: matcher});
        if (supported.indexOf(defaultLocale) === -1) {
            $ERROR("SupportedLocales didn't return default locale with matcher " + matcher + ".");
        }
        if (supported.indexOf(noLinguisticContent) !== -1) {
            $ERROR("SupportedLocales returned the \"no linguistic content\" locale with matcher " + matcher + ".");
        }
        if (supported.length > 1) {
            $ERROR("SupportedLocales returned stray locales: " + supported.join(", ") + " with matcher " + matcher + ".");
        }
    });

    return true;
});

