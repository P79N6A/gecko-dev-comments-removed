






































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



  if (typeof timeout == 'function')
  {
    expectExitCode(6);
    timeout(3);
    eval("do ([]); while(y for each (x in []))");
  }

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
