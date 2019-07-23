




































var gTestfile = 'regress-312588.js';

var BUGNUMBER = 312588;
var summary = 'Do not crash creating infinite array';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber(BUGNUMBER);
printStatus (summary);
expectExitCode(3)

  var a = new Array();

while (1)
{

  (a = new Array(a)).sort();
}

reportCompare(expect, actual, summary);
