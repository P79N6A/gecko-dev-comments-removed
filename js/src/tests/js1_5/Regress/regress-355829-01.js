





var BUGNUMBER = 355829;
var summary = 'Do not assert: !argc || argv[0].isNull() || JSVAL_IS_VOID(argv[0])';
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
