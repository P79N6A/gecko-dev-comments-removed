




































var gTestfile = 'regress-352073.js';

var BUGNUMBER = 352073;
var summary = 'decompilation of function expressions';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  var f;

  f = function() { (function x() {  }); return x; }
  expect = 'function() { return x; }';
  actual = f + '';
  compareSource(expect, actual, summary);

  f = function() {  (function(){} | x) }
  expect = 'function() {  (function(){} | x); } ';
  actual = f + '';
  compareSource(expect, actual, summary);

  exitFunc ('test');
}
