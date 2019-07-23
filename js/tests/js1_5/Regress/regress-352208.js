




































var bug = 352208;
var summary = 'Do not assert new Function("setter/*\n")';
var actual = 'No Crash';
var expect = 'No Crash';



test();


function test()
{
  enterFunc ('test');
  printBugNumber (bug);
  printStatus (summary);
  
  expect = 'SyntaxError: unterminated string literal';
  try
  {
    eval('new Function("setter/*\n");');
  }
  catch(ex)
  { 
    actual = ex + '';
  }
  reportCompare(expect, actual, 'new Function("setter/*\n");');

  try
  {
    eval('new Function("setter/*\n*/");');
  }
  catch(ex)
  { 
    actual = ex + '';
  }
  reportCompare(expect, actual, 'new Function("setter/*\n*/");');
  exitFunc ('test');
}
