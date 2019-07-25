





































var BUGNUMBER = 429264;
var summary = 'Do not assert: top < ss->printer->script->depth';
var actual = 'No Crash';
var expect = 'No Crash';


test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  function f() { for(; 1; ) { } }
  if (typeof trap == 'function' && typeof setDebug == 'function')
  {
    setDebug(true);
    trap(f, 0, "");
  }
  f + '';

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
