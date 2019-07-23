




































var gTestfile = 'regress-274152.js';

var BUGNUMBER = 274152;
var summary = 'Do not ignore unicode format-control characters';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
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
