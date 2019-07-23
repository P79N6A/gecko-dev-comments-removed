




































var gTestfile = 'regress-452498-092.js';

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



  expect = 'TypeError: redeclaration of formal parameter e';
  try
  {
    eval('(function (e) { var e; const e; });');
  }
  catch(ex)
  {
    actual = ex + '';
  }



  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
