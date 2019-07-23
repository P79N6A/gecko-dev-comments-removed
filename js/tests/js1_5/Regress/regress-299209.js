




































var bug = 299209;
var summary = 'anonymous function expression statement => JS stack overflow';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber (bug);
printStatus (summary);

try
{
  eval('for (a = 0; a <= 10000; a++) { function(){("");} }');
}
catch(e)
{
}
 
reportCompare(expect, actual, summary);
