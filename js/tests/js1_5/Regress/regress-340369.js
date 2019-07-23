





































var gTestfile = 'regress-340369.js';

var BUGNUMBER = 340369;
var summary = 'Oh for crying out loud.';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber(BUGNUMBER);
printStatus (summary);

try
{ 
  eval('return /;');
}
catch(ex)
{
  print(ex+'');
}

reportCompare(expect, actual, summary);
