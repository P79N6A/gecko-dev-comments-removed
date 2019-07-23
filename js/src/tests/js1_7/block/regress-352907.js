




































var gTestfile = 'regress-352907.js';

var BUGNUMBER = 352907;
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
    eval('(function() { while(0) while(0) let k=3; return k; })');
  }
  catch(ex)
  {
    actual = ex.name;
  }
 
  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
