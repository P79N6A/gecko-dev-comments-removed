





































var BUGNUMBER = 387501;
var summary =
  'Array.prototype.toString|toLocaleString are generic, ' +
  'Array.prototype.toSource is not generic';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  expect = '[object String]';
  actual = Array.prototype.toString.call((new String('foo')));
  assertEq(actual, expect, summary);

  expect = 'f,o,o';
  actual = Array.prototype.toLocaleString.call((new String('foo')));
  assertEq(actual, expect, summary);

  if (typeof Array.prototype.toSource != 'undefined')
  {
    try
    {
      expect = 'TypeError: Array.prototype.toSource called on incompatible String';
      actual = Array.prototype.toSource.call((new String('foo')));
    }
    catch(ex)
    {
      actual = ex + '';
    }
    assertEq(actual, expect, summary);
  }

  reportCompare(true, true, "Tests complete");

  exitFunc ('test');
}
