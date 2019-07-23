




































var gTestfile = 'regress-349619.js';

var BUGNUMBER = 349619;
var summary = 'Do not assert with let block, object literal getter/setter';
var actual = 'No Crash';
var expect = 'No Crash';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  (function() { let(y=3) { ({ get y() { }, set y(z) { } }) } });

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
