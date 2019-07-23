




































var gTestfile = 'regress-354297.js';

var BUGNUMBER = 354297;
var summary = 'getter/setter can be on index';
var actual = 'No Crash';
var expect = 'No Crash';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  print('This test requires GC_MARK_DEBUG');

  var o = {}; o.__defineGetter__(1, Math.sin); gc()

						 reportCompare(expect, actual, summary);

  exitFunc ('test');
}
