




































var gTestfile = 'regress-452498-030.js';

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



  function f() { var i = 0; var i = 5; }
  f();

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
