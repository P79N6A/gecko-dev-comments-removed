




































var gTestfile = 'regress-361571.js';

var BUGNUMBER = 361571;
var summary = 'Do not assert: fp->scopeChain == parent';
var actual = 'No Crash';
var expect = 'No Crash';


test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  try
  {
    o = {};
    o.__defineSetter__('y', eval);
    o.watch('y', function () { return "";});
    o.y = 1;
  }
  catch(ex)
  {
    printStatus('Note eval can no longer be called directly');
    expect = 'EvalError: function eval must be called directly, and not by way of a function of another name';
    actual = ex + '';
  }
  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
