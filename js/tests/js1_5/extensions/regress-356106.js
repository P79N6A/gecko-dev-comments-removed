




































var gTestfile = 'regress-356106.js';

var BUGNUMBER = 356106;
var summary = "Do not assert: rval[strlen(rval)-1] == '}'";
var actual = 'No Crash';
var expect = 'No Crash';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  (function() { return ({x setter: function(){} | 5 }) });

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
