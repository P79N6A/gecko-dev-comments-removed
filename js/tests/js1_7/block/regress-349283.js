




































var gTestfile = 'regress-349283.js';

var BUGNUMBER = 349283;
var summary = 'Do not crash with let statement in with block';
var actual = 'No Crash';
var expect = 'No Crash';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  with({b:2}) {
    let c = 3;
  }

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
