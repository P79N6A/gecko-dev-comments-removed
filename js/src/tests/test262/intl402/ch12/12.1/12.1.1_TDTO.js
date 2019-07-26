







$INCLUDE("testIntl.js");

var locales = [[], ["zh-Hans-CN"], ["hi-IN"], ["en-US"], ["id-ID"]];
var dates = [new Date(), new Date(0), new Date(Date.parse("1989-11-09T17:57:00Z"))];

function testWithDateTimeFormat(options, expected) {
    locales.forEach(function (locales) {
        var format = new Intl.DateTimeFormat(locales, options);
        var resolvedOptions = format.resolvedOptions();
        getDateTimeComponents().forEach(function (component) {
            if (resolvedOptions.hasOwnProperty(component)) {
                if (!expected.hasOwnProperty(component)) {
                    $ERROR("Unrequested component " + component +
                        " added to expected subset " + JSON.stringify(expected) +
                        "; locales " + locales + ", options " +
                        (options ? JSON.stringify(options) : options) + ".");
                }
            } else {
                if (expected.hasOwnProperty(component)) {
                    $ERROR("Missing component " + component +
                        " from expected subset " + JSON.stringify(expected) +
                        "; locales " + locales + ", options " +
                        (options ? JSON.stringify(options) : options) + ".");
                }
            }
        });
    });
}

function testWithToLocale(f, options, expected) {
    
    if (expected.length === undefined) {
        expected = [expected];
    }
    locales.forEach(function (locales) {
        dates.forEach(function (date) {
            var formatted = Date.prototype[f].call(date, locales, options);
            var expectedStrings = [];
            expected.forEach(function (expected) {
                var referenceFormat = new Intl.DateTimeFormat(locales, expected);
                expectedStrings.push(referenceFormat.format(date));
            });
            if (expectedStrings.indexOf(formatted) === -1) {
                $ERROR("Function " + f + " did not return expected string for locales " +
                    locales + ", options " + (options? JSON.stringify(options) : options) +
                    "; expected " +
                    (expectedStrings.length === 1 ? expectedStrings[0] : "one of " + expectedStrings) +
                    ", got " + formatted + ".");
            }
        });
    });
}   


testWithDateTimeFormat(undefined, {year: "numeric", month: "numeric", day: "numeric"});


testWithDateTimeFormat({year: "numeric", month: "numeric"}, {year: "numeric", month: "numeric"});


testWithDateTimeFormat({hour: "numeric", minute: "numeric"}, {hour: "numeric", minute: "numeric"});


testWithToLocale("toLocaleString", undefined, [
        
        {year: "numeric", month: "numeric", day: "numeric", hour: "numeric", minute: "numeric", second: "numeric"},
        {weekday: "short", year: "numeric", month: "numeric", day: "numeric", hour: "numeric", minute: "numeric", second: "numeric"}
]);


testWithToLocale("toLocaleString", {year: "numeric", month: "numeric"}, {year: "numeric", month: "numeric"});


testWithToLocale("toLocaleString", {hour: "numeric", minute: "numeric"}, {hour: "numeric", minute: "numeric"});


testWithToLocale("toLocaleDateString", undefined, {year: "numeric", month: "numeric", day: "numeric"});


testWithToLocale("toLocaleDateString", {year: "numeric", month: "numeric"}, {year: "numeric", month: "numeric"});


testWithToLocale("toLocaleDateString", {hour: "numeric", minute: "numeric", second: "numeric"}, [
        
        {year: "numeric", month: "numeric", day: "numeric", hour: "numeric", minute: "numeric", second: "numeric"},
        {weekday: "short", year: "numeric", month: "numeric", day: "numeric", hour: "numeric", minute: "numeric", second: "numeric"}
]);


testWithToLocale("toLocaleTimeString", undefined, {hour: "numeric", minute: "numeric", second: "numeric"});


testWithToLocale("toLocaleTimeString", {weekday: "short", year: "numeric", month: "numeric", day: "numeric"},
    {weekday: "short", year: "numeric", month: "numeric", day: "numeric", hour: "numeric", minute: "numeric", second: "numeric"});


testWithToLocale("toLocaleTimeString", {hour: "numeric", minute: "numeric"}, {hour: "numeric", minute: "numeric"});


