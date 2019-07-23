







































var gTestfile = 'regress-23346.js';

var CALL_CALLED = "PASSED";

test();

function f(x)
{
  if (x)
    return call();

  return "FAILED!";
}

function call()
{
  return CALL_CALLED;
}

function test()
{
  enterFunc ("test");

  printStatus ("ECMA Section: 10.1.3: Variable Instantiation.");
  printBugNumber (23346);

  reportCompare ("PASSED", f(true),
		 "Unqualified reference should not see Function.prototype");

  exitFunc("test");       
}
