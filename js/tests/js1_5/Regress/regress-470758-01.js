




































var gTestfile = 'regress-470758-01.js';

var BUGNUMBER = 470758;
var summary = 'Do not crash with eval upvars';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  (function() { var k; eval("for (var k in {});") })()

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
