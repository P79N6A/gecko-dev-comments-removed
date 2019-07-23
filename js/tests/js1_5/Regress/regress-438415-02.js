




































var gTestfile = 'regress-438415-02.js';

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
 
  expect = 'zero';
  Array.prototype[0] = 'zero';
  var a = [];
  a.length = 1;
  actual = a.pop();

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
