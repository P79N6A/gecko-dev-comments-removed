




































var bug = 294191;
var summary = 'Do not crash with nested function and "delete" op';
var actual = 'No Crash';
var expect = 'No Crash';

enterFunc ('test');
printBugNumber (bug);
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
