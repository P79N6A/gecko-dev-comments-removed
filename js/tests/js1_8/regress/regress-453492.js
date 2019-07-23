




































var gTestfile = 'regress-453492.js';

var BUGNUMBER = 453492;
var summary = 'Do not assert: op == JSOP_ENUMELEM || op == JSOP_ENUMCONSTELEM';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  print(function() { [(let(a)1)[2]] = 3; });

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
