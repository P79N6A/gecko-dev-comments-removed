





































var BUGNUMBER = 351515;
var summary = 'Invalid uses of yield, let keywords in js17';
var actual = '';
var expect = '';



test();


try
{
  expect = 'SyntaxError: syntax error';
  eval('yield = 1;');
  actual = 'No Error';
}
catch(ex)
{
  actual = ex + '';
}
reportCompare(expect, actual, summary + ': global: yield = 1');

try
{
  expect = 'SyntaxError: missing variable name';
  eval('let = 1;');
  actual = 'No Error';
}
catch(ex)
{
  actual = ex + '';
}
reportCompare(expect, actual, summary + ': global: let = 1');

function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  try
  {
    expect = 'SyntaxError: missing formal parameter';
    eval('function f(yield, let) { return yield+let; }');
    actual = 'No Error';
  }
  catch(ex)
  {
    actual = ex + '';
  }
  reportCompare(expect, actual, summary +
		': function f(yield, let) { return yield+let; }');

  try
  {
    expect = 'SyntaxError: missing variable name';
    eval('var yield = 1;');
    actual = 'No Error';
  }
  catch(ex)
  {
    actual = ex + '';
  }
  reportCompare(expect, actual, summary + ': function () {var yield;}');

  try
  {
    expect = 'SyntaxError: missing variable name';
    eval('var let = 1;');
    actual = 'No Error';
  }
  catch(ex)
  {
    actual = ex + '';
  }
  reportCompare(expect, actual, summary + ': function () { var let;}');

  try
  {
    expect = 'No Error';
    function yield() {}
    actual = 'No Error';
  }
  catch(ex)
  {
    actual = ex + '';
  }
  reportCompare(expect, actual, summary + ': function yield()');

  exitFunc ('test');
}
