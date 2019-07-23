




































var gTestfile = 'regress-353214.js';

var BUGNUMBER = 353214;
var summary = 'decompilation of |function() { (function ([x]) { })(); eval("return 3;") }|';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  var f = function() { (function ([x]) { })(); eval('return 3;') }
  expect = 'function() { (function ([x]) { }()); eval("return 3;"); }';
  actual = f + '';
  compareSource(expect, actual, summary);

  exitFunc ('test');
}
