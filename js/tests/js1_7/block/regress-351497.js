




































var gTestfile = 'regress-351497.js';

var BUGNUMBER = 351497;
var summary = 'Do not assert for(let (w) x in y)';
var actual = 'No Crash';
var expect = 'No Crash';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  expect = 'SyntaxError: invalid for/in left-hand side';
  try
  {
    eval('for(let (w) x in y) { }');
  }
  catch(ex)
  {
    actual = ex + '';
  }

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
