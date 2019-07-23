




































var bug = 274152;
var summary = 'Do not ignore unicode format-control characters';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber (bug);
  printStatus (summary);
  
  expect = 'SyntaxError: illegal character';
  try
  {
    eval("hi\uFEFFthere = 'howdie';");
  }
  catch(ex)
  {
    actual = ex + '';
  }

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
