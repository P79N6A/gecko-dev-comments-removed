




































var gTestfile = 'regress-350730.js';

var BUGNUMBER = 350730;
var summary = 'Assertion: pn2->pn_slot >= 0 || varOrConst [@ EmitVariables]';
var actual = 'No Crash';
var expect = 'No Crash';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  with({}) let y;

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
