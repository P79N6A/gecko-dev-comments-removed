




































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

  var formatcontrolchars = ['\u200C',
                            '\u200D',
                            '\u200E', 
                            '\u0600', 
                            '\u0601', 
                            '\u0602', 
                            '\u0603', 
                            '\u06DD', 
                            '\u070F'];

  for (var i = 0; i < formatcontrolchars.length; i++)
  {
    try
    {
      eval("hi" + formatcontrolchars[i] + "there = 'howdie';");
    }
    catch(ex)
    {
      actual = ex + '';
    }

    reportCompare(expect, actual, summary + ': ' + i);
  }

  exitFunc ('test');
}
