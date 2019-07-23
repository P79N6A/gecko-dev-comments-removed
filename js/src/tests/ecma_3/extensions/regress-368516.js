




































var gTestfile = 'regress-368516.js';

var BUGNUMBER = 368516;
var summary = 'Treat unicode BOM characters as whitespace';
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
    expect = 'howdie';
    actual = '';

    try
    {
      eval("var" + bomchars[i] + "hithere = 'howdie';");
      actual = hithere;
    }
    catch(ex)
    {
      actual = ex + '';
    }

    reportCompare(expect, actual, summary + ': ' + i);
  }

  exitFunc ('test');
}
