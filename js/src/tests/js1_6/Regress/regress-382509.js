





































var BUGNUMBER = 382509;
var summary = 'Disallow non-global indirect eval';
var actual = '';
var expect = '';

var global = typeof window == 'undefined' ? this : window;
var object = {};


test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  global.foo = eval;
  global.a   = 'global';
  expect = 'global indirect';
  actual = String(['a+" indirect"'].map(global.foo));
  reportCompare(expect, actual, summary + ': global indirect');

  object.foo = eval;
  object.a   = 'local';
  expect = 'EvalError: function eval must be called directly, and not by way of a function of another name';
  try
  {
    actual = String(['a+" indirect"'].map(object.foo, object));
  }
  catch(ex)
  {
    actual = ex + '';
  }
  reportCompare(expect, actual, summary + ': local indirect');

  exitFunc ('test');
}
