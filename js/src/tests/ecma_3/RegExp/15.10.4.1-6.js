












































var gTestfile = '15.10.4.1-6.js';

var BUGNUMBER = 476940;
var summary = 'Section 15.10.4.1 - RegExp with invalid flags';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  var invalidflags = ['ii', 'gg', 'mm', 'a'];

  for (var i = 0; i < invalidflags.length; i++)
  {
    var flag = invalidflags[i];
    expect = 'SyntaxError: invalid regular expression flag ' + flag.charAt(0);
    actual = '';
    try
    {
      new RegExp('bar', flag);
    }
    catch(ex)
    {
      actual = ex + '';
    }
    reportCompare(expect, actual, summary + 
                  ': new RegExp("bar", "' + flag + '")');

    actual = '';
    try
    {
      eval("/bar/" + flag);
    }
    catch(ex)
    {
      actual = ex + '';
    }
    reportCompare(expect, actual, summary + ': /bar/' + flag + ')');
  }

  exitFunc ('test');
}
