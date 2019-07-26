





var BUGNUMBER = 355829;
var summary = 'Do not assert: !argc || argv[0].isNull() || argv[0].isUndefined()';
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
