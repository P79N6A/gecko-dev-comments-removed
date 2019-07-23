






































var gTestfile = 'regress-257751.js';

var BUGNUMBER = 257751;
var summary = 'RegExp Syntax Errors should have lineNumber and fileName';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
printStatus (summary);

var status;
var re;

status = summary + ' ' + inSection(1) + ' RegExp("\\\\") ';
try
{
  expect = 'Pass';
  re = RegExp('\\');
}
catch(e)
{
  if (e.fileName && e.lineNumber)
  {
    actual = 'Pass';
  }
  else
  {
    actual = 'Fail';
  }
}
reportCompare(expect, actual, status);

status = summary + ' ' + inSection(2) + ' RegExp(")") ';
try
{
  expect = 'Pass';
  re = RegExp(')');
}
catch(e)
{
  if (e.fileName && e.lineNumber)
  {
    actual = 'Pass';
  }
  else
  {
    actual = 'Fail';
  }
}
reportCompare(expect, actual, status);

status = summary + ' ' + inSection(3) + ' /\\\\/ ';
try
{
  expect = 'Pass';
  re = eval('/\\/');
}
catch(e)
{
  if (e.fileName && e.lineNumber)
  {
    actual = 'Pass';
  }
  else
  {
    actual = 'Fail';
  }
}
reportCompare(expect, actual, status);

status = summary + ' ' + inSection(4) + ' /)/ ';
try
{
  expect = 'Pass';
  re = eval('/)/');
}
catch(e)
{
  if (e.fileName && e.lineNumber)
  {
    actual = 'Pass';
  }
  else
  {
    actual = 'Fail';
  }
}
reportCompare(expect, actual, status);
