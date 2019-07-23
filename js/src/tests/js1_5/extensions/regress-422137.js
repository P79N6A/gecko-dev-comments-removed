




































var gTestfile = 'regress-422137.js';

var BUGNUMBER = 422137;
var summary = 'Do not assert or bogo OOM with debugger trap on JOF_CALL bytecode';
var actual = 'No Crash';
var expect = 'No Crash';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  function f() { return a(); }

  if (typeof trap == 'function')
  {
    trap(f, 0, "print('trap')");
  }
  f + '';

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
