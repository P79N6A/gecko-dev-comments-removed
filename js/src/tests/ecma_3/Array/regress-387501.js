




































var gTestfile = 'regress-387501.js';


var BUGNUMBER = 387501;
var summary = 'Array.prototype.toString|toSource|toLocaleString is not generic';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  try
  {
    expect = 'TypeError: Array.prototype.toString called on incompatible String';
    actual = Array.prototype.toString.call((new String('foo')));
  }
  catch(ex)
  {
    actual = ex + '';
  }
  reportCompare(expect, actual, summary);

  try
  {
    expect = 'TypeError: Array.prototype.toLocaleString called on incompatible String';
    actual = Array.prototype.toLocaleString.call((new String('foo')));
  }
  catch(ex)
  {
    actual = ex + '';
  }
  reportCompare(expect, actual, summary);

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
    reportCompare(expect, actual, summary);
  }

  exitFunc ('test');
}
