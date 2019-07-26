







$INCLUDE("testIntl.js");

getDateTimeComponents().forEach(function (component) {
    testOption(Intl.DateTimeFormat, component, "string", getDateTimeComponentValues(component), undefined, {isILD: true});
});

