





var BUGNUMBER = 349507;
var summary = 'Do not assert with let block, let statement and const';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  try
  {
    eval('(function() { let(x = 1) { const b = 2 }; let b = 3; })');
  }
  catch(ex)
  {
    actual = ex + '';
  }

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
