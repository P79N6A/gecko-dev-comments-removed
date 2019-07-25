






































var BUGNUMBER = 683738;
var summary = 'return with argument and lazy generator detection';
var actual = '';
var expect = '';


test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  expect = "generator function foo returns a value";
  try
  {
    actual = 'No Error';
    eval("function foo(x) { if (x) { return this; } else { yield 3; } }");
  }
  catch(ex)
  {
    actual = ex.message;
  }
  reportCompare(expect, actual, summary + ": 1");

  expect = "generator function foo returns a value";
  try
  {
    actual = 'No Error';
    eval("function foo(x) { if (x) { yield 3; } else { return this; } }");
  }
  catch(ex)
  {
    actual = ex.message;
  }
  reportCompare(expect, actual, summary + ": 2");

  expect = "generator function foo returns a value";
  try
  {
    actual = 'No Error';
    eval("function foo(x) { if (x) { return this; } else { (yield 3); } }");
  }
  catch(ex)
  {
    actual = ex.message;
  }
  reportCompare(expect, actual, summary + ": 3");

  expect = "generator function foo returns a value";
  try
  {
    actual = 'No Error';
    eval("function foo(x) { if (x) { (yield 3); } else { return this; } }");
  }
  catch(ex)
  {
    actual = ex.message;
  }
  reportCompare(expect, actual, summary + ": 4");

}
