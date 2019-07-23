




































var gTestfile = 'regress-419803.js';

var BUGNUMBER = 419803;
var summary = 'Do not assert: sprop->parent == scope->lastProp';
var actual = 'No Crash';
var expect = 'No Crash';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  for (var i=0; i<2; ++i) ({ p: 5, p: 7 });

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
