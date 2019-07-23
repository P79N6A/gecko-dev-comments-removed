





































var gTestfile = 'regress-3649-n.js';


var BUGNUMBER = 3649;
var summary = 'gc-checking branch callback.';
var actual = 'error';
var expect = 'error';

DESCRIPTION = summary;
EXPECTED = expect;

printBugNumber(BUGNUMBER);
printStatus (summary);
 
expectExitCode(0);
expectExitCode(5);

var s = "";
s = "abcd";
for (i = 0; i < 100000; i++)  {
  s += s;
}

expect = 'No Crash';
actual = 'No Crash';

reportCompare(expect, actual, summary);
