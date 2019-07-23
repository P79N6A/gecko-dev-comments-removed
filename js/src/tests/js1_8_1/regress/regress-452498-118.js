




































var gTestfile = 'regress-452498-118.js';

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



  (function() { (function() { e *= 4; })(); var e; })();




  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
