




































var gTestfile = 'regress-352197.js';

var BUGNUMBER = 352197;
var summary = 'TypeError if yield after return value in a block';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  expect = /TypeError: anonymous generator function returns a value/;
  try
  {
    var gen = eval('(function() { { return 5; } yield 3; })');
    actual = 'No Error';
  }
  catch(ex)
  {
    actual = ex + '';
  }
  reportMatch(expect, actual, summary);

  exitFunc ('test');
}
