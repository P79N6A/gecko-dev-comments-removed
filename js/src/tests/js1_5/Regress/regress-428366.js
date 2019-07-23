






var gTestfile = 'regress-428366.js';

var BUGNUMBER = 428366;
var summary = 'Do not assert deleting eval 16 times';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
printStatus (summary);

this.__proto__.x = eval;
for (i = 0; i < 16; ++i) delete eval;
(function w() { x = 1; })();
 
reportCompare(expect, actual, summary);
