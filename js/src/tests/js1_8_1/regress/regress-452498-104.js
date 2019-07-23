




































var gTestfile = 'regress-452498-104.js';

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



  (function(a) { function b() { a; } function a() { } })();


  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
