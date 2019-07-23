




































var gTestfile = 'regress-407501.js';

var BUGNUMBER = 407501;
var summary = 'JSOP_NEWINIT lacks SAVE_SP_AND_PC ';
var actual = 'No Crash';
var expect = 'No Crash';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  if (typeof gczeal == 'function')
  {
    gczeal(2);
  }

  var a = [[[[[[[0]]]]]]];
  if (uneval(a).length == 0)
    throw "Unexpected result";
 
  if (typeof gczeal == 'function')
  {
    gczeal(0);
  }

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}

