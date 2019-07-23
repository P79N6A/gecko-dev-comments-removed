




































var gTestfile = 'regress-379483.js';

var BUGNUMBER = 379483;
var summary = 'Do not assert: top < ss->printer->script->depth';
var actual = 'No Crash';
var expect = 'No Crash';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  (function () { try { } catch([e]) { [1]; } });

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
