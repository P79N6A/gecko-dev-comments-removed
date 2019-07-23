




































var gTestfile = 'regress-355635.js';

var BUGNUMBER = 355635;
var summary = 'decompilation of let binding nothing';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  var f = function () { let ([] = []) { } }
  expect = 'function () { let ([] = []) { } }';
  actual = f + '';
  compareSource(expect, actual, summary);

  exitFunc ('test');
}
