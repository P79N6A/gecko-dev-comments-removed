





var BUGNUMBER = 455981;
var summary = 'Do not assert: entry->localKind == JSLOCAL_ARG';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  expect = 'SyntaxError: duplicate argument names not allowed in this context';

  try
  {
    eval('(function ({a, b, c, d, e, f, g, h, q}, q) { })');
  }
  catch(ex)
  {
    actual = ex + '';
  }
  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
