




































var gTestfile = 'regress-376564.js';

var BUGNUMBER = 376564;
var summary = 'Decompilation of new (eval())';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  var f = function() { new (eval()) };
  expect = 'function() { new (eval()); }';
  actual = f + '';
  compareSource(expect, actual, summary);

  f = function() { new (g()) };
  expect = 'function() { new (g()); }';
  actual = f + '';
  compareSource(expect, actual, summary);

  exitFunc ('test');
}
