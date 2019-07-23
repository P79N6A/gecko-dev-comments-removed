





































var gTestfile = 'regress-295666.js';

var BUGNUMBER = 295666;
var summary = 'Check JS only recursion stack overflow';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber(BUGNUMBER);
printStatus (summary);

try
{
  throw {toString: parseInt.call};
}
catch(e)
{
} 
reportCompare(expect, actual, summary);
