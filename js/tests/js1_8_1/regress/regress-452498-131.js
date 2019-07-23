




































var gTestfile = 'regress-452498-131.js';

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





  try
  {
    eval('((__defineGetter__, function (x) { function x(){} }) for');
  }
  catch(ex)
  {
  }


  try
  {
    eval('( ""  ? 1.3 : x); *::*; x::x;');
  }
  catch(ex)
  {
  }



  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
