






var gTestfile = 'regress-10278.js';








var BUGNUMBER = 10278;
var summary = 'Function declarations do not need to be separated by semi-colon';
var actual;
var expect;



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  expect = 'pass';
  try
  {
    eval("function f(){}function g(){}");
    actual = "pass";
    printStatus('no exception thrown');
  }
  catch ( e )
  {
    actual = "fail";
    printStatus('exception ' + e.toString() + ' thrown');
  }

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
