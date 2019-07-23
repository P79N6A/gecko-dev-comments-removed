




































var gTestfile = 'regress-349012-02.js';

var BUGNUMBER = 349012;
var summary = 'generators with nested try finally blocks';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  expect = "[object StopIteration]";
  var expectyield   = "12";
  var expectfinally = "Inner finally,Outer finally";
  var actualyield = "";
  var actualfinally = "";

  function gen()
  {
    try {
      try {
        yield 1;
      } finally {
        actualfinally += "Inner finally";
        yield 2;
      }
    } finally {
      actualfinally += ",Outer finally";
    }
  }

  var iter = gen();
  actualyield += iter.next();
  actualyield += iter.next();
  try
  {
    actualyield += iter.next();
    actual = "No exception";
  }
  catch(ex)
  {
    actual = ex + '';
  }
 
  reportCompare(expect, actual, summary);
  reportCompare(expectyield, actualyield, summary);
  reportCompare(expectfinally, actualfinally, summary);

  exitFunc ('test');
}
