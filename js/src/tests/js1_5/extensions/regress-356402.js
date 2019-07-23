




































var gTestfile = 'regress-356402.js';

var BUGNUMBER = 356402;
var summary = 'Do not assert: slot < fp->nvars';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber(BUGNUMBER);
printStatus (summary);

if (typeof Script == 'undefined')
{
  print('Test skipped. Script not defined.');
}
else
{ 
  (function() { new Script('for(var x in x) { }')(); })();
}
reportCompare(expect, actual, summary);
