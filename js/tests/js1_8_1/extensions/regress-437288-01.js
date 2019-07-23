




































var gTestfile = 'regress-437288-01.js';

var BUGNUMBER = 437288;
var summary = 'for loop turning into a while loop';
var actual = 'No Hang';
var expect = 'No Hang';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  expect = 'SyntaxError: invalid for/in left-hand side';
  try
  {
    eval('(function() { const x = 1; for (x in null); })();');
  }
  catch(ex)
  {
    actual = ex + '';
  }

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
