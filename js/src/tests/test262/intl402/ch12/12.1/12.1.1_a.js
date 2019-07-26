








$INCLUDE("testIntl.js");

testForUnwantedRegExpChanges(function () {
    new Intl.DateTimeFormat("de-DE-u-ca-gregory");
});

testForUnwantedRegExpChanges(function () {
    new Intl.DateTimeFormat("de-DE-u-ca-gregory", {timeZone: "UTC"});
});
