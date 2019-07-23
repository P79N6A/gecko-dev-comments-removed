




































var gTestfile = 'regress-452498-006.js';

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



  function foo() {
    var x = 4;
    var f = (function() { return x++; });
    var g = (function() { return x++; });
    return [f,g];
  }

  var bar = foo();

  expect = '9';
  actual = 0;

  bar[0]();
  bar[1]();

  actual = String(expect);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
