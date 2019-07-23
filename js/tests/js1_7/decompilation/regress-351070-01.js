




































var bug = 351070;
var summary = 'decompilation of let declaration should not change scope';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber (bug);
  printStatus (summary);
  
  var f = function () { var a = 2; if (!!true) let a = 3; return a; }
  expect = 'function () { var a = 2; if (!!true) let a = 3; return a; }';
  actual = f + '';
  compareSource(expect, actual, summary);

  expect = 3;
  actual = f();
  reportCompare(expect, actual, summary);

  f = function () { var a = 2; if (!!true) {let a = 3;} return a; }
  expect = 'function () { var a = 2; if (!!true) { let a = 3;} return a; }';
  actual = f + '';
  compareSource(expect, actual, summary);

  expect = 2;
  actual = f();
  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
