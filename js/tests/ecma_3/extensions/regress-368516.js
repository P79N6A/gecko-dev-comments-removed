




































var gTestfile = 'regress-368516.js';

var BUGNUMBER = 368516;
var summary = 'Ignore unicode BOM characters';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  var bomchars = ['\uFFFE',
                  '\uFEFF'];

  for (var i = 0; i < bomchars.length; i++)
  {
    expect = 'No Error';
    actual = 'No Error';

    try
    {
      eval("hi" + bomchars[i] + "there = 'howdie';");
    }
    catch(ex)
    {
      actual = ex + '';
    }

    reportCompare(expect, actual, summary + ': ' + i);
  }

  exitFunc ('test');
}
