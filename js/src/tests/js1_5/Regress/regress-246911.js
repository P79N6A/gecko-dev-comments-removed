





































var gTestfile = 'regress-246911.js';

var BUGNUMBER = 246911;
var summary = 'switch() statement with variable as label';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
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
