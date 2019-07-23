




































var gTestfile = 'regress-304897.js';

var BUGNUMBER = 304897;
var summary = 'uneval("\\t"), uneval("\\x09")';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
printStatus (summary);

expect = '"\\t"';
actual = uneval('\t'); 
reportCompare(expect, actual, summary);

actual = uneval('\x09'); 
reportCompare(expect, actual, summary);
