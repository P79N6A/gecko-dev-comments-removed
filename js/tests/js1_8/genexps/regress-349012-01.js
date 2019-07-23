




































var gTestfile = 'regress-349012-01.js';

var BUGNUMBER = 349012;
var summary = 'closing a generator fails to report error if yield during close is ignored';
var actual = '';
var expect = '';



test();


if (typeof quit != 'undefined')
{
  quit(0);
}

function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  expect = "Inner finally,Outer finally";

  function gen()
  {
    try {
      try {
        yield 1;
      } finally {
        actual += "Inner finally";
        yield 2;
      }
    } finally {
      actual += ",Outer finally";
    }
  }

  try {
    for (var i in gen())
      break;
  } catch (e) {
    if (!(e instanceof TypeError))
      throw e;
  }

  reportCompare(expect, actual, summary);
  exitFunc ('test');
}
