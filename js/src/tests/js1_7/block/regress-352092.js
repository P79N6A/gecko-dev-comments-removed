




































var gTestfile = 'regress-352092.js';

var BUGNUMBER = 352092;
var summary = 'Do not assert on with() let';
var actual = 'No Crash';
var expect = 'No Crash';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  try
  { 
    eval('let(z) { with({}) let y = 3; }');
  }
  catch(ex)
  {
    
    summary = 'let declaration must be direct child of block or top-level implicit block';
    expect = 'SyntaxError';
    actual = ex.name;
  }

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
