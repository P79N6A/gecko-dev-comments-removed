



















































































































var gTestfile = 'regress-157652.js';
var BUGNUMBER = 157652;
var summary = "Testing that Array.sort() doesn't crash on very large arrays";
var expect = 'No Crash';
var actual = 'No Crash';

printBugNumber(BUGNUMBER);
printStatus(summary);

expectExitCode(0);
expectExitCode(5);

var IN_RHINO = inRhino();

try
{
  if (!IN_RHINO)
  {
    var a1=Array(0xFFFFFFFF);
    a1.sort();
    a1 = null;
  }

  var a2 = Array(0x40000000);
  a2.sort();
  a2=null;

  var a3=Array(0x10000000/4);
  a3.sort();
  a3=null;
}
catch(ex)
{
  
  expect = 'InternalError: allocation size overflow';
  actual = ex + '';
}

reportCompare(expect, actual, summary);
