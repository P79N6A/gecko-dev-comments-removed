




































var gTestfile = 'regress-452498-116.js';

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




  (new Function("for (var x = 0; x < 3; ++x) { (function(){})() } "))();



  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
