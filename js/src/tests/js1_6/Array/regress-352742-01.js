




































var gTestfile = 'regress-352742-01.js';

var BUGNUMBER = 352742;
var summary = 'Array filter on {valueOf: Function}';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  print('If the test harness fails, this test fails.');
  expect = 4; 
  z = {valueOf: Function};
  actual = 2;
  try {
    [11].filter(z);
  }
  catch(e)
  {
    actual = 3;
    print(e);
  }
  actual = 4;

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
