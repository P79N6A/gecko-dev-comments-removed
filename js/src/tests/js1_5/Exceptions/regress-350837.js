




































var gTestfile = 'regress-350837.js';

var BUGNUMBER = 350837;
var summary = 'clear cx->throwing in finally';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  expect = 'F';

  function f()
  {
    actual = "F";
  }

  try
  {
    try {
      throw 1;
    } finally {
      f.call(this);
    }
  }
  catch(ex)
  {
    reportCompare(1, ex, summary);
  }

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
