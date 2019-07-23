




































var gTestfile = 'regress-455981-02.js';

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

  expect = 'SyntaxError: duplicate argument is mixed with destructuring pattern';

  try
  {
    eval('(function ({a: {b: bb, c: cc, d: dd}, m: [x, n, o, p]}, x) {});');
  }
  catch(ex)
  {
    actual = ex + '';
  }
  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
