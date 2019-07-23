




































var gTestfile = 'regress-410649.js';

var BUGNUMBER = 410649;
var summary = 'function statement and destructuring parameter name clash';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  function f(a) {
    function a() { }
    return a;
  }

  function g([a, b]) {
    function a() { }
    return a;
  }

  expect = 'function';
  actual = typeof f(1);
  reportCompare(expect, actual, "type for simple parameter case");

  expect = 'function';
  actual = typeof g([1, 2]);
  reportCompare(expect, actual, "type for destructuring parameter case");

  exitFunc ('test');
}
