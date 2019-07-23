




































var gTestfile = 'regress-346203.js';

var BUGNUMBER = 346203;
var summary = 'Do not crash during destructuring assignment ';
var actual = 'No Crash';
var expect = 'No Crash';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  var {b:{c:x}}={b:{c:1}}

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
