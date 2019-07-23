




































var gTestfile = 'regress-349298.js';

var BUGNUMBER = 349298;
var summary = 'Do not bogo assert';
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
    eval('(function() { for(i=0;i<4;++i) let x = 4; })');
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
