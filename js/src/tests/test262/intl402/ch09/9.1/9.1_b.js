








$INCLUDE("testIntl.js");

testWithIntlConstructors(function (Constructor) {
    var info = getLocaleSupportInfo(Constructor);
    var fallback;
    info.supported.forEach(function (locale) {
        var pos = locale.lastIndexOf("-");
        if (pos !== -1) {
            fallback = locale.substring(0, pos);
            if (info.supported.indexOf(fallback) === -1) {
                $ERROR("Locale " + locale + " is supported, but fallback " + fallback + " isn't.");
            }
        }
        var match = /([a-z]{2,3})(-[A-Z][a-z]{3})(-[A-Z]{2})/.exec(locale);
        if (match !== null) {
            fallback = match[1] + match[3];
            if (info.supported.indexOf(fallback) === -1) {
                $ERROR("Locale " + locale + " is supported, but fallback " + fallback + " isn't.");
            }
        }
    });
});

