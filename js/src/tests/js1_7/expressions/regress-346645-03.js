




































var gTestfile = 'regress-346645-03.js';

var BUGNUMBER = 346645;
var summary = 'Do not crash with non-empty array in destructuring assign LHS';
var actual = 'No Crash';
var expect = 'No Crash';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  try
  {
    eval('({ a:[z] }) = 3;');
  }
  catch(ex)
  {
    print(ex);
  }

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
