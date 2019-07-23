




































var gTestfile = 'regress-334807-04.js';

var BUGNUMBER = 334807;
var summary = '10.1.8 - arguments prototype is the original Object prototype.';
var actual = 'No Error';
var expect = 'TypeError';

printBugNumber(BUGNUMBER);
printStatus (summary);

printStatus('set Object = Array');

Object = Array;

try
{
  0, function () { printStatus( arguments.join()); }( 1, 2, 3 ); 
}
catch(ex)
{
  printStatus(ex + '');
  actual = ex.name;
}
reportCompare(expect, actual, summary);
