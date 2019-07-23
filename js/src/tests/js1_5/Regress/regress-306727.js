




































var gTestfile = 'regress-306727.js';

var BUGNUMBER = 306727;
var summary = 'Parsing RegExp of octal expressions in strict mode';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
printStatus (summary);



try
{
  expect = null;
  actual = /.\011/.exec ('a'+String.fromCharCode(0)+'11');
}
catch(e)
{
}

reportCompare(expect, actual, summary);


options('strict');

expect = null;
try
{
  actual = /.\011/.exec ('a'+String.fromCharCode(0)+'11');
}
catch(e)
{
}

reportCompare(expect, actual, summary);
