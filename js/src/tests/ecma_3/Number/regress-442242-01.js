




































var gTestfile = 'regress-442242-01.js';

var BUGNUMBER = 442242;
var summary = 'Do not assert: INT_FITS_IN_JSVAL(i)';
var actual = 'No Crash';
var expect = 'No Crash';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  var i = 28800000;
  -i;

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
