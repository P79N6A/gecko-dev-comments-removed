




































var bug = 352742;
var summary = 'Array filter on {valueOf: Function}';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber (bug);
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
