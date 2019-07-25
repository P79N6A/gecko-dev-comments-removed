






































var BUGNUMBER = 326467;
var summary = 'Do not assert: slot < fp->nvars, at jsinterp.c';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber(BUGNUMBER);
printStatus (summary);

eval('for(var prop in {1:1})prop;', {})
 
  reportCompare(expect, actual, summary);
