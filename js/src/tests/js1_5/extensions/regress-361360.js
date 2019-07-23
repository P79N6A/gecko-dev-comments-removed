




































var gTestfile = 'regress-361360.js';

var BUGNUMBER = 361360;
var summary = 'Do not assert: !caller || caller->pc involving setter and watch';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  expect = actual = 'No Crash';

  this.__defineSetter__('x', eval);
  this.watch('x', function(){});
  x = 3;

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
