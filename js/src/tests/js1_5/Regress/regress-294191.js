




































var gTestfile = 'regress-294191.js';

var BUGNUMBER = 294191;
var summary = 'Do not crash with nested function and "delete" op';
var actual = 'No Crash';
var expect = 'No Crash';

enterFunc ('test');
printBugNumber(BUGNUMBER);
printStatus (summary);
 
function f()
{
  function x()
  {
    x;
  }
}

f.z=0;

delete f.x;

f();

reportCompare(expect, actual, summary);
