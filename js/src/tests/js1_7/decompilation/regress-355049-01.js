




































var gTestfile = 'regress-355049-01.js';

var BUGNUMBER = 355049;
var summary = 'decompilation of destructing into two hole';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  var f = function () { [3 for ([, , ] in null)] }
  expect = 'function () { [3 for ([, , ] in null)]; }';
  actual = f + '';
  compareSource(expect, actual, summary);

  exitFunc ('test');
}
