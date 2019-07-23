




































var gTestfile = 'regress-352281.js';

var BUGNUMBER = 352281;
var summary = 'decompilation of |while| and function declaration';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  var f, g;
  f = function() { { while(0) function t() {  } } }
  expect = 'function() { while(0) { function t() {  } }}';
  actual = f + '';
  compareSource(expect, actual, summary);

  g = eval(uneval(actual));
  actual = g + '';
  compareSource(expect, actual, summary);

  exitFunc ('test');
}
