




































var gTestfile = 'regress-352261.js';

var BUGNUMBER = 352261;
var summary = 'Decompilation should preserve right associativity';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  var g, h;

  g = function(a,b,c) { return a - (b + c) }
  expect = 'function(a,b,c) { return a - (b + c); }';
  actual = g + '';
  compareSource(expect, actual, summary);

  h = eval(uneval(g));
  expect = g(1, 10, 100);
  actual = h(1, 10, 100);
  reportCompare(expect, actual, summary);

  var p, q;

  p = function (a,b,c) { return a + (b - c) }
  expect = 'function (a,b,c) { return a + (b - c);}';
  actual = p + '';
  compareSource(expect, actual, summary);

  q = eval(uneval(p));
  expect = p(3, "4", "5");
  actual = q(3, "4", "5");
  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
