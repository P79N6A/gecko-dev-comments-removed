




































var gTestfile = 'regress-351070-03.js';

var BUGNUMBER = 351070;
var summary = 'decompilation of let declaration should not change scope';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  var f;

  f = function (x){if (x) if (y) z; else let w }
  expect = 'function (x){if (x) {if (y) { z; } else let w; }}';
  actual = f + '';
  compareSource(expect, actual, summary);

  f = function (x){if (x){ if (y) z;} else let w  }
  expect = 'function (x){if (x){ if (y) {z;}} else let w;  }';
  actual = f + '';
  compareSource(expect, actual, summary);

  f = function (x){if (x){ if (y) let z;} else let w  }
  expect = 'function (x){if (x){ if (y) let z;} else let w;  }';
  actual = f + '';
  compareSource(expect, actual, summary);

  f = function f(){var a = 2; if (x) {let a = 3; print(a)} return a}
  expect = 'function f(){var a = 2; if (x) {let a = 3; print(a);} return a;}';
  actual = f + '';
  compareSource(expect, actual, summary);

  f = function f(){var a = 2; if (x) {print(a);let a = 3} return a}
  expect = 'function f(){var a = 2; if (x) {print(a);let a = 3;} return a;}';
  actual = f + '';
  compareSource(expect, actual, summary);

  f = function f(){var a = 2; if (x) {let a = 3} return a}
  expect = 'function f(){var a = 2; if (x) {let a = 3;} return a;}';
  actual = f + '';
  compareSource(expect, actual, summary);

  f = function f(){var a = 2; if (x) let a = 3; return a}
  expect = 'function f(){var a = 2; if (x) let a = 3; return a;}';
  actual = f + '';
  compareSource(expect, actual, summary);

  exitFunc ('test');
}
