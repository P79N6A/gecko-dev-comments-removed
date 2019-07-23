




































var gTestfile = 'regress-106244.js';

var BUGNUMBER = 106244;
var summary = 'No warning in strict mode if (a = b && c == d)...';
var actual = '';
var expect = 'test for equality (==) mistyped as assignment (=)?';

printBugNumber(BUGNUMBER);
printStatus (summary);

options('strict');
options('werror');

var a = false;
var b = true;
var c = false;
var d = true;
var result;

try
{
  if (a = b && c == d)
    result = true;
  else
    result = false;
}
catch(ex)
{
  actual = ex.message;
}

print('result = ' + result);

reportCompare(expect, actual, summary);
