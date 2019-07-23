




































var gTestfile = 'regress-349491.js';

var BUGNUMBER = 349491;
var summary = 'Incorrect decompilation due to assign to const';
var actual = 'No Crash';
var expect = 'No Crash';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  var a;

  a = function () { const z = 3; g = 7; g = z += 1; return g };
  expect = a();
  actual = (eval('(' + a + ')'))();
  reportCompare(expect, actual, summary);

  a = function () { const z = 3; return z += 2 };
  expect = a();
  actual = (eval('(' + a + ')'))();
  reportCompare(expect, actual, summary);

  expect = 'function () {\n    const z = 3;\n}';
  a  = function () { const z = 3; z = 4; }
  actual = a.toString();
  compareSource(expect, actual, summary);

  expect = 'function () {\n    const z = 3;\n}';
  a  = function () { const z = 3; 4; }
  actual = a.toString();
  compareSource(expect, actual, summary);

  exitFunc ('test');
}
