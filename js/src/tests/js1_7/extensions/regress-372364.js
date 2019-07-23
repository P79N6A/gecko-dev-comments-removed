




































var gTestfile = 'regress-372364.js';

var BUGNUMBER = 372364;
var summary = 'Do not recurse to death on (function() { yield ([15].some([].watch)); })().next()';
var actual = 'No Crash';
var expect = 'No Crash';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  print('FIXME: Fix this test to check the proper error when bug 366669 is fixed');
  try
  {
    (function() { yield ([15].some([].watch)); })().next();
  }
  catch(ex)
  {
    print(ex);
  }

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
