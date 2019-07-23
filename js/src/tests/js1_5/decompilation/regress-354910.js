




































var gTestfile = 'regress-354910.js';

var BUGNUMBER = 354910;
var summary = 'decompilation of (delete r(s)).t = a;';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  var f = function () { (delete r(s)).t = a; }

  expect = 'function () { (r(s), true).t = a; }';
  actual = f + '';
  compareSource(expect, actual, summary);

  exitFunc ('test');
}
