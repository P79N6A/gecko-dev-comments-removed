




































var gTestfile = 'regress-356083.js';

var BUGNUMBER = 356083;
var summary = 'decompilation for ({this setter: function () { } }) ';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  var f = function() { return {this setter: function () { } }; } ;
  expect = 'function() { return {this setter: function () { } }; }';
  actual = f + '';

  compareSource(expect, actual, summary);

  expect = "({'' setter:(function () {})})";
  actual = uneval({'' setter: function(){}});
  reportCompare(expect, actual, expect);
  exitFunc ('test');
}
