




































var gTestfile = 'regress-431428.js';

var BUGNUMBER = 431428;
var summary = 'Do not crash with for..in, trap';
var actual = 'No Crash';
var expect = 'No Crash';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  function f() { 
    for ( var a in [0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17]) { }
  }

  if (typeof trap == 'function')
  {
    "" + f;
    trap(f, 0, "");
    "" + f;
  }

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
