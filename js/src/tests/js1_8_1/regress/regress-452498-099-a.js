




































var gTestfile = 'regress-452498-099-a.js';

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

    while( getter = function() { return y } for (y in y) )( /x/g );
  }




  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
