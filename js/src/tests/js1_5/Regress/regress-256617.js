





































var gTestfile = 'regress-256617.js';


var BUGNUMBER = 256617;
var summary = 'throw statement: eol should not be allowed';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
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
