





































var gTestfile = 'regress-449657.js';

var BUGNUMBER = 449657;
var summary = 'JS_SealObject on Arrays';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  if (typeof seal != 'function')
  {
    expect = actual = 'JS_SealObject not supported, test skipped.';
    reportCompare(expect, actual, summary);
  }
  else
  {
    try
    {
      var a= [1, 2, 3];
      seal(a);
    }
    catch(ex)
    {
      actual = ex + '';
    }
    reportCompare(expect, actual, summary + ': 1');

    expect = 'Error: a.length is read-only';
    actual = '';
    try
    {
      a = [1,2,3];
      a[4] = 2;
      seal(a);
      a.length = 5;
    }
    catch(ex)
    {
      actual = ex + '';
    }
    reportCompare(expect, actual, summary + ': 2');
  }

  exitFunc ('test');
}
