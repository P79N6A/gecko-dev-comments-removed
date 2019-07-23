




































var gTestfile = 'regress-306633.js';

var BUGNUMBER = 306727;
var summary = 'report compile warnings in evald code when strict warnings enabled';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
printStatus (summary);

options('strict');
options('werror');

expect = 'SyntaxError';

try
{
  actual = eval('super = 5');
}
catch(e)
{
  actual = e.name;
}

reportCompare(expect, actual, summary);
