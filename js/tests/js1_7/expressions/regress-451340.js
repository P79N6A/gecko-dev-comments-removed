




































var gTestfile = 'regress-451340.js';

var BUGNUMBER = 451340;
var summary = 'Do no crash [@ CheckDestructuring]';
var actual = 'No Crash';
var expect = 'No Crash';


test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  function x([y]) { }

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
