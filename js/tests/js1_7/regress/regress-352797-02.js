




































var gTestfile = 'regress-352797-02.js';

var BUGNUMBER = 352797;
var summary = 'Assertion: OBJ_GET_CLASS(cx, obj) == &js_BlockClass';
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
    (function() { let (x = eval.call(<x/>.(1), "")) {} })();
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
