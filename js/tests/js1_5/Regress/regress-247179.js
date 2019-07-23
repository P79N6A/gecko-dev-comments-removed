






































var gTestfile = 'regress-247179.js';

var BUGNUMBER = 247179;
var summary = 'RegExp \\b should not recognize non-ASCII alphanumerics as word characters';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
printStatus (summary);
 
expect = 3;
actual = "m\ucc44nd".split(/\b/).length;
 
reportCompare(expect, actual, summary);
