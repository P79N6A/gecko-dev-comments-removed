




































var gTestfile = 'regress-355829-01.js';

var BUGNUMBER = 355829;
var summary = 'Do not assert: !argc || JSVAL_IS_NULL(argv[0]) || JSVAL_IS_VOID(argv[0])';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  new Object({valueOf: /a/});

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
