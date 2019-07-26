








$INCLUDE("testIntl.js");


var locales = ["tlh", "id", "en"];
var a = ["A", "C", "E", "B", "D", "F"];
var referenceCollator = new Intl.Collator(locales);
var referenceSorted = a.slice().sort(referenceCollator.compare);

function MyCollator(locales, options) {
    Intl.Collator.call(this, locales, options);
    
}

MyCollator.prototype = Object.create(Intl.Collator.prototype);
MyCollator.prototype.constructor = MyCollator;


var collator = new MyCollator(locales);
a.sort(collator.compare);
testArraysAreSame(referenceSorted, a);

