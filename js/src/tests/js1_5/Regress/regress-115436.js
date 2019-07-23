





































var gTestfile = 'regress-115436.js';

var BUGNUMBER = 115436;
var summary = 'Do not crash javascript warning duplicate arguments';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber(BUGNUMBER);
printStatus (summary);

options('strict');

function x(y,y)
{
  return 3;
}

var z = x(4,5);

reportCompare(expect, actual, summary);
