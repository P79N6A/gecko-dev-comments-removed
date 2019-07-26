








$INCLUDE("testIntl.js");

testForUnwantedRegExpChanges(function () {
    new Intl.Collator("de-DE-u-co-phonebk");
});
