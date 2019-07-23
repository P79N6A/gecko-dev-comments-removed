




































var gTestfile = 'regress-438415-01.js';

var BUGNUMBER = 438415;
var summary = 'Do not assert: *vp != JSVAL_HOLE';
var actual = 'No Crash';
var expect = 'No Crash';


test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  [1,,].pop();

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
