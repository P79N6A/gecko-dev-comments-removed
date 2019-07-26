








$INCLUDE("testIntl.js");

testWithIntlConstructors(function (Constructor) {
    var info = getLocaleSupportInfo(Constructor);
    
    ["lookup", "best fit"].forEach(function (matcher) {
        var supportedByConstructor = info.supported.concat(info.byFallback);
        var supported = Constructor.supportedLocalesOf(supportedByConstructor,
            {localeMatcher: matcher});
        
        var i = 0;
        var limit = Math.min(supportedByConstructor.length, supported.length);
        while (i < limit && supportedByConstructor[i] === supported[i]) {
            i++;
        }
        if (i < supportedByConstructor.length) {
            $ERROR("Locale " + supportedByConstructor[i] +
                " is returned by resolvedOptions but not by supportedLocalesOf.");
        } else if (i < supported.length) {
            $ERROR("Locale " + supported[i] +
                " is returned by supportedLocalesOf but not by resolvedOptions.");
        }
    });
    
    
    var unsupportedByConstructor = info.unsupported;
    var supported = Constructor.supportedLocalesOf(unsupportedByConstructor,
            {localeMatcher: "lookup"});
    if (supported.length > 0) {
        $ERROR("Locale " + supported[0] +
            " is returned by supportedLocalesOf but not by resolvedOptions.");
    }

    return true;
});

