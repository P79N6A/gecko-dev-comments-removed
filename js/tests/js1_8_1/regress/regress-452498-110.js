




































var gTestfile = 'regress-452498-110.js';

var BUGNUMBER = 452498;
var summary = 'TM: upvar2 regression tests';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);





  function f(a) { const b = a; print(++b); return b; }

  expect = 'function f(a) { const b = a; print(+ b + 1); return b; }';
  actual = f + '';
  compareSource(expect, actual, 'function f(a) { const b = a; print(++b); return b; }');

  expect = '01';
  actual = 0;

  function g(a) { const b = a; print(actual = ++b); return b; }
  actual = String(actual) + g(1);
  reportCompare(expect, actual, 'function g(a) { const b = a; print(actual = ++b); return b; }');

  expect = '21';
  actual = 0;

  const x = 1; print(actual = ++x);
  actual = String(actual) + x;

  reportCompare(expect, actual, 'const x = 1; print(actual = ++x); ');

  exitFunc ('test');
}
