




































var gTestfile = 'regress-443071-01.js';

var BUGNUMBER = 443071;
var summary = 'Do not assert: top != 0 with for (;;[]=[])';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  print(function() { for (;;[]=[]) { } });

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
