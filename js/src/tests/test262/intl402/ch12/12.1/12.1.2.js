








$INCLUDE("testIntl.js");


var locales = ["tlh", "id", "en"];
var a = [new Date(0), Date.now(), new Date(Date.parse("1989-11-09T17:57:00Z"))];
var referenceDateTimeFormat = new Intl.DateTimeFormat(locales);
var referenceFormatted = a.map(referenceDateTimeFormat.format);

function MyDateTimeFormat(locales, options) {
    Intl.DateTimeFormat.call(this, locales, options);
    
}

MyDateTimeFormat.prototype = Object.create(Intl.DateTimeFormat.prototype);
MyDateTimeFormat.prototype.constructor = MyDateTimeFormat;


var format = new MyDateTimeFormat(locales);
var actual = a.map(format.format);
testArraysAreSame(referenceFormatted, actual);

