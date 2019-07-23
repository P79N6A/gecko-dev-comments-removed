




































var gTestfile = 'regress-412324.js';

var BUGNUMBER = 412324;
var summary = 'Allow function Error(){} for the love of Pete';
var actual = 'No Error';
var expect = 'No Error';

printBugNumber(BUGNUMBER);
printStatus (summary);

function Error() {}

reportCompare(expect, actual, summary);
