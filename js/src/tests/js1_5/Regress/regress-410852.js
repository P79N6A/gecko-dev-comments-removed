







var BUGNUMBER = 410852;
var summary = 'Valgrind errors in jsemit.cpp';



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
    print(ex + "");
    assertEq(ex instanceof SyntaxError, true, "wrong error: " + ex);
  }

  reportCompare(true, true, summary);

  exitFunc ('test');
}
