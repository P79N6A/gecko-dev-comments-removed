







$INCLUDE("testIntl.js");

testOption(Intl.DateTimeFormat, "hour12", "boolean", undefined, undefined,
    {extra: {any: {hour: "numeric", minute: "numeric"}}});
testOption(Intl.DateTimeFormat, "hour12", "boolean", undefined, undefined,
    {noReturn: true});

