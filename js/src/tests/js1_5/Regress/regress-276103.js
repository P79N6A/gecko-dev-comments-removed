





































var gTestfile = 'regress-276103.js';


var BUGNUMBER = 276103;
var summary = 'link foo and null bytes';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
printStatus (summary);
 

var testString = "test|string";
var idx = testString.indexOf("|");
var link = testString.substring(0, idx);
var desc = testString.substring(idx + 1);

expect = '<a href="test">string</a>';
actual = desc.link(link);

reportCompare(expect, actual, summary);

