




































var gTestfile = 'regress-354246.js';

var BUGNUMBER = 354246;
var summary = 'calling Error constructor with object with bad toString';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  expect = '13';

  actual += '1';
  try
  {
    new Error({toString: function() { x.y } });
  }
  catch(e)
  {
  }
  actual += '3';
  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
