






































var bug = 256617;
var summary = 'throw statement: eol should not be allowed';
var actual = '';
var expect = '';

  printBugNumber (bug);
  printStatus (summary);

expect = 'syntax error';

try
{
  eval('throw\n1;');
  actual = 'throw ignored';
}
catch(e)
{
  if (e instanceof SyntaxError)
  {
    actual = 'syntax error';
  }
  else
  {
    actual = 'no error';
  }
}
  
  reportCompare(expect, actual, summary);
