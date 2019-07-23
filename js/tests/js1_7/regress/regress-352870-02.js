




































var bug = 352870;
var summary = 'Do not assert for crazy huge testcases';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber (bug);
  printStatus (summary);
  
  expect = 'TypeError: [1, 2, 3, 4].g has no properties';
  actual = '';
  try
  {
    switch(4) { case [(let (y = [].j(5)) ({})) 
                      for (p in ([1,2,3,4].g).v({},((w).y(z, <x/>))))]: }  }
  catch(ex)
  {
    actual = ex + '';
  }
  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
