





































var gTestfile = 'regress-271716-n.js';

var BUGNUMBER = 271716;
var summary = 'Don\'t Crash on infinite loop creating new Arrays';
var actual = 'Crash';
var expect = 'No Crash';

printBugNumber(BUGNUMBER);
printStatus (summary);
 
try
{
  a = new Array();
  while (1) a = new Array(a);
  actual = 'No Crash';
}
catch(e)
{
  actual = 'No Crash';
}

reportCompare(expect, actual, summary);
