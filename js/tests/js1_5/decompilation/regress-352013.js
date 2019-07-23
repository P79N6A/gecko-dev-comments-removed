




































var gTestfile = 'regress-352013.js';

var BUGNUMBER = 352013;
var summary = 'decompilation of new parenthetic expressions';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  var f, g, h;
  var x = Function;
  var z = 'actual += arguments[0];';
  var w = 42;

  f = function() { new (x(z))(w) }
  expect = 'function() { new (x(z))(w); }';
  actual = f + '';
  compareSource(expect, actual, summary);

  g = function () { new x(z)(w); }
  expect = 'function () { (new x(z))(w); }';
  actual = g + '';
  compareSource(expect, actual, summary);

  expect = '4242';
  actual = '';
  f();
  g();
  reportCompare(expect, actual, summary);

  h = function () { new (x(y)(z));  }
  expect = 'function () { new (x(y)(z)); }';
  actual = h + '';
  compareSource(expect, actual, summary);

  h = function () { new (x(y).z);   }
  expect = 'function () { new (x(y).z); }';
  actual = h + '';
  compareSource(expect, actual, summary);

  h = function () { new x(y).z;   }
  expect = 'function () { (new x(y)).z; }';
  actual = h + '';
  compareSource(expect, actual, summary);

  exitFunc ('test');
}
