





var BUGNUMBER = 465443;
var summary = '';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  expect = 'SyntaxError: invalid assignment to const b';

  jit(true);

  try
  {
    eval('(function () { const b = 16; var out = []; for each (b in [true, "", true, "", true, ""]) out.push(b >> 1) })();');
  }
  catch(ex)
  {
    actual = ex + '';
  }

  jit(false);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
