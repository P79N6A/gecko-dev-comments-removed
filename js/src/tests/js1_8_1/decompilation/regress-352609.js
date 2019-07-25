





































var BUGNUMBER = 352609;
var summary = 'decompilation of |let| expression for |is not a function| error';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  expect = /TypeError: 0 is not a function/;
  try
  {
    [let (x = 3, y = 4) x].map(0);
  }
  catch(ex)
  {
    actual = ex + '';
  }
  reportMatch(expect, actual, '[let (x = 3, y = 4) x].map(0)');

  expect = /TypeError: (p.z = \[let \(x = 3, y = 4\) x\]|.*Array.*) is not a function/;
  try
  {
    var p = {}; (p.z = [let (x = 3, y = 4) x])();
  }
  catch(ex)
  {
    actual = ex + '';
  }
  reportMatch(expect, actual, 'p = {}; (p.z = [let (x = 3, y = 4) x])()');

  expect = /TypeError: (p.z = let \(x\) x|.*Undefined.*) is not a function/;
  try
  {
    var p = {}; (p.z = let(x) x)()
  }
  catch(ex)
  {
    actual = ex + '';
  }
  reportMatch(expect, actual, 'p = {}; (p.z = let(x) x)()');

  exitFunc ('test');
}
