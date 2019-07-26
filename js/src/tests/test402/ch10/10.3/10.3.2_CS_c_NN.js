









$INCLUDE("testIntl.js");


var locales = ["de-DE-u-co-phonebk", "de-u-co-phonebk"];
var collator = new Intl.Collator(locales, {localeMatcher: "lookup"});
if (locales.indexOf(collator.resolvedOptions().locale) !== -1) {
    var a = ["A", "b", "Af", "Ab", "od", "off", "Ä", "ö"];
    a.sort(collator.compare);
    var expected = ["A", "Ab", "Ä", "Af", "b", "od", "ö", "off"];
    testArraysAreSame(expected, a);
}

