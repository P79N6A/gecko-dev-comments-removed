




































var gTestfile = 'regress-352870-01.js';

var BUGNUMBER = 352870;
var summary = 'Do not assert for crazy huge gTestcases';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  expect = 'ReferenceError: g is not defined';
  actual = '';
  try
  {
    switch(4) { case [([11,12,13,14].v ? 2 : let (a=1,b=2) 5)
                      for (c in [<x/> for (f in g)])]: }
  }
  catch(ex)
  {
    actual = ex + '';
  }
  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
