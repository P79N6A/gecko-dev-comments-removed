




































var gTestfile = '10.1.3-2.js';

var BUGNUMBER = 395907;
var summary = 'eval of function declaration should change existing variable';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  expect = 'typeof x: number, typeof x: function, x(): true';
  var x = "string";
  function f () {
    var x = 0;
    actual += 'typeof x: ' + (typeof x) + ', ';
    eval('function x() { return true; }');
    actual += 'typeof x: ' + (typeof x) + ', ';
    actual += 'x(): ' + x();
  }
  f();

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
