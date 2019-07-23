




































var gTestfile = 'regress-417893.js';

var BUGNUMBER = 417893;
var summary = 'Fast natives must use JS_THIS/JS_THIS_OBJECT';
var actual = 'No Crash';
var expect = 'No Crash';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  (function() { var s = function(){}.prototype.toSource; s(); })();

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
