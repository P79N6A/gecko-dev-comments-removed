




































var gTestfile = 'regress-424683-01.js';

var BUGNUMBER = 424683;
var summary = 'Throw too much recursion instead of script stack space quota';
var actual = 'No Error';
var expect = 'InternalError: too much recursion';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  function f() { f(); }

  try
  {
    f();
  }
  catch(ex)
  {
    actual = ex + '';
  }
  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
