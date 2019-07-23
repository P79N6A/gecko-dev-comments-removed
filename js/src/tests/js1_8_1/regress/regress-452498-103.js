




































var gTestfile = 'regress-452498-103.js';

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



  (function(a) { v = 5; let [v] = [3]; (function(){ v; })(); })();


  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
