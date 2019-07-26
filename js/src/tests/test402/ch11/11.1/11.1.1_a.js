








$INCLUDE("testIntl.js");

testForUnwantedRegExpChanges(function () {
    new Intl.NumberFormat("de-DE-u-nu-latn");
});

testForUnwantedRegExpChanges(function () {
    new Intl.NumberFormat("de-DE-u-nu-latn", {style: "currency", currency: "EUR"});
});
