





var BUGNUMBER = 452498;
var summary = 'TM: upvar2 regression tests';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);



  expect = 'ReferenceError: can\'t access lexical declaration `q\' before initialization';

  try
  {
    q = new Function("(function() { q(3); })(); const q = undefined;"); q();
  }
  catch(ex)
  {
    actual = ex + '';
  }

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
