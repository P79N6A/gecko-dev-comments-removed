




































var gTestfile = 'regress-385729.js';

var BUGNUMBER = 385729;
var summary = 'uneval(eval(expression closure))';
var actual = 'No Crash';
var expect = 'No Crash';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  if (typeof eval != 'undefined' && typeof uneval != 'undefined')
  {
    expect = '(function f () /x/g)';
    try
    {
      
      actual = uneval(eval(expect));
    }
    catch(ex)
    {
      
      expect = 'SyntaxError: missing { before function body';
      actual = ex + '';
    }
    compareSource(expect, actual, summary);

    expect = '({get f () /x/g})';
    try
    {
      
      actual = uneval(eval("({get f () /x/g})"));
    }
    catch(ex)
    {
      
      expect = 'SyntaxError: missing { before function body';
      actual = ex + '';
    }
    compareSource(expect, actual, summary);
  }

  exitFunc ('test');
}
