




































var gTestfile = 'regress-452498-178.js';

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



  eval("with({}) let(x=[])(function(){#2=x})()");

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
