





































var gTestfile = 'regress-112626.js';

var BUGNUMBER = 112626;
var summary = 'Do not crash String.split(regexp) when regexp contains parens';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber(BUGNUMBER);
printStatus (summary);

var _cs='2001-01-01';
var curTime = _cs.split(/([- :])/);
 
reportCompare(expect, actual, summary);
