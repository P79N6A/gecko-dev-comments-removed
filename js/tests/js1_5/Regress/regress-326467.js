





































var bug = 326467;
var summary = 'Assertion failure: slot < fp->nvars, at jsinterp.c';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber (bug);
printStatus (summary);

eval('for(var prop in {1:1})prop;', {})
  
reportCompare(expect, actual, summary);
