






var gTestfile = 'regress-452498-054.js';

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







  function f() { var x; eval("let x, x;"); }
  f();




  eval("let(x) function(){ x = this; }()");

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
