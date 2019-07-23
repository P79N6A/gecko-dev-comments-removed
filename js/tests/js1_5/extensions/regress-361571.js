




































var gTestfile = 'regress-361571.js';

var BUGNUMBER = 361571;
var summary = 'Assertion: fp->scopeChain == parent';
var actual = 'No Crash';
var expect = 'No Crash';


test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  o = {};
  o.__defineSetter__('y', eval);
  o.watch('y', function () { return "";});
  o.y = 1;

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
