




































var gTestfile = 'regress-410852.js';

var BUGNUMBER = 410852;
var summary = 'Valgrind errors in jsemit.c';
var actual = '';
var expect = 'SyntaxError: syntax error';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  print('Note: You must run this test under valgrind to determine if it passes');

  try
  {
    eval('function(){if(t)');
  }
  catch(ex)
  {
    actual = ex + '';
    print(actual);
  }

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
