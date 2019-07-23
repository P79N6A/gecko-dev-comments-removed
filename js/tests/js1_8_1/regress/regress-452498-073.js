




































var gTestfile = 'regress-452498-073.js';

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
    eval('function() { var arguments }');
  }
  catch(ex)
  {
  }



  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
