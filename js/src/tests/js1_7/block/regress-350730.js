




































var gTestfile = 'regress-350730.js';

var BUGNUMBER = 350730;
var summary = 'Do not assert: pn2->pn_slot >= 0 || varOrConst [@ EmitVariables]';
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
    eval('with({}) let y;');
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
