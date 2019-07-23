




































var gTestfile = 'regress-452498-111.js';

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
 


options("anonfunfix");
new Function("{function x(){}}");



  reportCompare(expect, actual, summary);

  exitFunc ('test');
}


