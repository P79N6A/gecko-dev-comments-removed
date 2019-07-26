







$INCLUDE("testIntl.js");

testWithIntlConstructors(function (Constructor) {
    var defaultLocale = new Constructor().resolvedOptions().locale;
    var supportedLocales = Constructor.supportedLocalesOf([defaultLocale]);
    if (supportedLocales.indexOf(defaultLocale) === -1) {
        $ERROR("Default locale is not reported as available.");
    }
});

