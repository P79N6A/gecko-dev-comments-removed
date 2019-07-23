




































var gTestfile = 'regress-420869-01.js';

var BUGNUMBER = 420869;
var summary = 'Throw too much recursion instead of script stack space quota';
var actual = 'No Error';
var expect = 'InternalError: too much recursion';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  function f(i) {
    if (i == 0) 
      return 1; 
    return i*f(i-1);
  }

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
