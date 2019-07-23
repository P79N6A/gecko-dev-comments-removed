




































var gTestfile = 'regress-424257.js';

var BUGNUMBER = 424257;
var summary = 'Do not assert: op2 == JSOP_INITELEM';
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
    eval("var x; while(x getter={});");
  }
  catch(ex)
  {
    expect = 'SyntaxError: invalid getter usage';
    actual = ex + '';
  }

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
