





































var gTestfile = 'regress-261886.js';

var BUGNUMBER = 261886;
var summary = 'Always evaluate delete operand expression';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
printStatus (summary);

var o = {a:1};

expect = 2;
try
{
  delete ++o.a;
  actual = o.a;
}
catch(e)
{
  actual = o.a;
  summary += ' ' + e;
}
 
reportCompare(expect, actual, summary);
