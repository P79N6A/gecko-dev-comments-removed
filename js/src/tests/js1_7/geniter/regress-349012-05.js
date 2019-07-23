




































var gTestfile = 'regress-349012-05.js';

var BUGNUMBER = 349012;
var summary = 'generator recursively calling itself via close is an Error';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  var iter;
  function gen() {
    iter.close();
    yield 1;
  }

  expect = /TypeError.*[aA]lready executing generator/;
  try
  {
    iter = gen();
    var i = iter.next();
    print("i="+i);
  }
  catch(ex)
  {
    print(ex + '');
    actual = ex + '';
  }
  reportMatch(expect, actual, summary);

  exitFunc ('test');
}
