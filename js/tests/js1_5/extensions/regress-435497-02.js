





































var gTestfile = 'regress-435497-02.js';

var BUGNUMBER = 435497;
var summary = 'Do not assert op2 == JSOP_INITELEM';
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
    eval('(function() { x setter = 0, y; const x; })();');
  }
  catch(ex)
  {
  }

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
