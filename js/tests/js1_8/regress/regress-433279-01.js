




































var gTestfile = 'regress-433279-01.js';

var BUGNUMBER = 433279;
var summary = 'Do not assert: pn != tc->parseContext->nodeList';
var actual = 'No Crash';
var expect = 'No Crash';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  var { sin, PI } = Math; sin(PI / 2);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
