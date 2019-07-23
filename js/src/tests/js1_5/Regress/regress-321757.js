





































var gTestfile = 'regress-321757.js';

var BUGNUMBER = 321757;
var summary = 'Compound assignment operators should not bind LHS';
var actual = '';
var expect = 'pass';

printBugNumber(BUGNUMBER);
printStatus (summary);

try
{
  foo += 1;
  actual = "fail";
}
catch (e)
{
  actual = "pass";
}
 
reportCompare(expect, actual, summary + ': +=');

try
{
  foo -= 1;
  actual = "fail";
}
catch (e)
{
  actual = "pass";
}
 
reportCompare(expect, actual, summary + ': -=');

try
{
  foo *= 1;
  actual = "fail";
}
catch (e)
{
  actual = "pass";
}
 
reportCompare(expect, actual, summary + ': *=');

try
{
  foo /= 1;
  actual = "fail";
}
catch (e)
{
  actual = "pass";
}
 
reportCompare(expect, actual, summary + ': /=');

try
{
  foo %= 1;
  actual = "fail";
}
catch (e)
{
  actual = "pass";
}
 
reportCompare(expect, actual, summary + ': %=');

try
{
  foo <<= 1;
  actual = "fail";
}
catch (e)
{
  actual = "pass";
}
 
reportCompare(expect, actual, summary + ': <<=');

try
{
  foo >>= 1;
  actual = "fail";
}
catch (e)
{
  actual = "pass";
}
 
reportCompare(expect, actual, summary + ': >>=');

try
{
  foo >>>= 1;
  actual = "fail";
}
catch (e)
{
  actual = "pass";
}
 
reportCompare(expect, actual, summary + ': >>>=');
