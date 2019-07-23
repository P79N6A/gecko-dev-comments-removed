





































var bug = 246911;
var summary = 'switch() statement with variable as label';
var actual = '';
var expect = '';

printBugNumber (bug);
printStatus (summary);
  
expect = 'PASS';

var a = 10;
a = 9;
var b = 10;

switch(b)
{
case a:
  actual = 'FAIL';
  break;
default:
  actual = 'PASS';
}

reportCompare(expect, actual, summary);
