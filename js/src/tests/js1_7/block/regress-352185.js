




































var gTestfile = 'regress-352185.js';

var BUGNUMBER = 352185;
var summary = 'Do not assert on switch with let';
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
    eval('switch(let (a) 2) { case 0: let b; }');
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
