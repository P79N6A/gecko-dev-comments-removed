




































var gTestfile = 'regress-352870-02.js';

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
 
  expect = /TypeError: \[1, 2, 3, 4\].g (has no properties|is undefined)/;
  actual = '';
  try
  {
    switch(4) { case [(let (y = [].j(5)) ({}))
                      for (p in ([1,2,3,4].g).v({},((w).y(z, <x/>))))]: }  }
  catch(ex)
  {
    actual = ex + '';
  }
  reportMatch(expect, actual, summary);

  exitFunc ('test');
}
