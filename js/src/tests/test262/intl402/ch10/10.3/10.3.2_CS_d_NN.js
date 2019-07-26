









$INCLUDE("testIntl.js");


var locales = ["de", "en", "es", "it"];
locales = Intl.Collator.supportedLocalesOf(locales, {localeMatcher: "lookup"});
locales.forEach(function (locale) {
    var target = "Aa";
    var input = ["Aa", "bA", "E", "b", "aA", "fC", "áÁ", "Aã"];
    var expected = {
        "base": ["Aa", "aA", "áÁ", "Aã"],
        "accent": ["Aa", "aA"],
        "case": ["Aa", "Aã"],
        "variant": ["Aa"]
    };
    Object.getOwnPropertyNames(expected).forEach(function (sensitivity) {
        var collator = new Intl.Collator([locale], {usage: "search",
                sensitivity: sensitivity, localeMatcher: "lookup"});
        var matches = input.filter(function (v) {
            return collator.compare(v, target) === 0;
        });
        testArraysAreSame(expected[sensitivity], matches);
    });
});

