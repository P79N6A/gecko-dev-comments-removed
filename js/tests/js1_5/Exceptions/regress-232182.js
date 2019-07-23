




































var gTestfile = 'regress-232182.js';


var BUGNUMBER = 232182;
var summary = 'Display non-ascii characters in JS exceptions';
var actual = '';
var expect = 'no error';

printBugNumber(BUGNUMBER);
printStatus (summary);







var utf8Enabled = false;
try
{
  \u0441\u0442;
}
catch (e)
{
  utf8Enabled = (e.message.charAt (0) == '\u0441');
}



printStatus('UTF-8 is ' + (utf8Enabled?'':'not ') + 'enabled');

if (!utf8Enabled)
{
  reportCompare('Not run', 'Not run', 'utf8 is not enabled');
}
else
{
  status = summary + ': Throw Error with Unicode message';
  expect = 'test \u0440\u0441';
  try
  {
    throw Error (expect);
  }
  catch (e)
  {
    actual = e.message;
  }
  reportCompare(expect, actual, status);

  var inShell = (typeof stringsAreUTF8 == "function");
  if (!inShell)
  {
    inShell = (typeof stringsAreUtf8  == "function");
    if (inShell)
    {
      this.stringsAreUTF8 = stringsAreUtf8;
      this.testUTF8 = testUtf8;
    }
  }

  if (inShell && stringsAreUTF8())
  {
    status = summary + ': UTF-8 test: bad UTF-08 sequence';
    expect = 'Error';
    actual = 'No error!';
    try
    {
      testUTF8(1);
    }
    catch (e)
    {
      actual = 'Error';
    }
    reportCompare(expect, actual, status);

    status = summary + ': UTF-8 character too big to fit into Unicode surrogate pairs';
    expect = 'Error';
    actual = 'No error!';
    try
    {
      testUTF8(2);
    }
    catch (e)
    {
      actual = 'Error';
    }
    reportCompare(expect, actual, status);

    status = summary + ': bad Unicode surrogate character';
    expect = 'Error';
    actual = 'No error!';
    try
    {
      testUTF8(3);
    }
    catch (e)
    {
      actual = 'Error';
    }
    reportCompare(expect, actual, status);
  }

  if (inShell)
  {
    status = summary + ': conversion target buffer overrun';
    expect = 'Error';
    actual = 'No error!';
    try
    {
      testUTF8(4);
    }
    catch (e)
    {
      actual = 'Error';
    }
    reportCompare(expect, actual, status);
  }
}
