




































var gTestfile = 'regress-352044-02-n.js';

var BUGNUMBER = 352044;
var summary = 'issues with Unicode escape sequences in JavaScript source code';
var actual = 'No Error';
var expect = 'SyntaxError';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  print('This test case is expected to throw an uncaught SyntaxError');

  try
  {
    var i = 1;
    i \u002b= 1; 
    print(i);
  }
  catch(ex)
  {
    actual = ex + '';
  }

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
