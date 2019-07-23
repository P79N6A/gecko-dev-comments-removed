





































var gTestfile = 'regress-435497-03.js';

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
    eval('(function() { x getter= function(){} ; var x5, x = 0x99; })();');
  }
  catch(ex)
  {
  }

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
