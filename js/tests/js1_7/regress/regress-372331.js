




































var gTestfile = 'regress-372331.js';

var BUGNUMBER = 372331;
var summary = 'for-in should not bind name too early';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  var index;
  var obj = { index: 1 };

  expect = 'No Error';
  actual = 'No Error';

  function gen()
  {
    delete obj.index;
    yield 2;
  }

  with (obj) {
    for (index in gen());
  }

  try
  {
    if ('index' in obj)
      throw "for-in binds name to early";

    if (index !== 2)
      throw "unexpected value of index: "+index;
  }
  catch(ex)
  {
    actual = ex + '';
  }

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
