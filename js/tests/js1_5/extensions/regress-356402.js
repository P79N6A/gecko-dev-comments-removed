




































var bug = 356402;
var summary = 'Assertion failure: slot < fp->nvars';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber (bug);
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
