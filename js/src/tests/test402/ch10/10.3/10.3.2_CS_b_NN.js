








$INCLUDE("testIntl.js");


var locales = ["de", "en", "es", "fr", "it"];
locales = Intl.Collator.supportedLocalesOf(locales, {localeMatcher: "lookup"});
locales.forEach(function (locale) {
    var collator = new Intl.Collator([locale], {sensitivity: "variant", localeMatcher: "lookup"});
    var a = ["L", "X", "C", "k", "Z", "H", "d", "m", "w", "A", "i", "f", "y", "E", "N", "V", "g", "J", "b"];
    a.sort(collator.compare);
    var expected = ["A", "b", "C", "d", "E", "f", "g", "H", "i", "J", "k", "L", "m", "N", "V", "w", "X", "y", "Z"];
    testArraysAreSame(expected, a);
});

