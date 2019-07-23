




































var gTestfile = 'regress-352606.js';

var BUGNUMBER = 352606;
var summary = 'Do not crash involving post decrement';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  y = ({toString: gc}); new Function("y--;")()

			  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
