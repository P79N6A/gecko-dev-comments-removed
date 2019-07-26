








$INCLUDE("testIntl.js");


var locales = ["tlh", "id", "en"];
var a = [0, 1, -1, -123456.789, -Infinity, NaN];
var referenceNumberFormat = new Intl.NumberFormat(locales);
var referenceFormatted = a.map(referenceNumberFormat.format);

function MyNumberFormat(locales, options) {
    Intl.NumberFormat.call(this, locales, options);
    
}

MyNumberFormat.prototype = Object.create(Intl.NumberFormat.prototype);
MyNumberFormat.prototype.constructor = MyNumberFormat;


var format = new MyNumberFormat(locales);
var actual = a.map(format.format);
testArraysAreSame(referenceFormatted, actual);

