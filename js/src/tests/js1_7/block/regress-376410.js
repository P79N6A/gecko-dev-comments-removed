




































var gTestfile = 'regress-376410.js';

var BUGNUMBER = 376410;
var summary = 'let declaration must be direct child of block, ' + 
  'top-level implicit block, or switch body block';
var actual = '';
var expect = 'SyntaxError';




test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  try
  {
    eval('(function() { while(0) L: let x; print(x); })');
  }
  catch(ex)
  {
    actual = ex.name;
  }
 
  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
