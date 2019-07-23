




































var gTestfile = 'regress-320887.js';

var BUGNUMBER = 320887;
var summary = 'var x should not throw a ReferenceError';
var actual = 'No error';
var expect = 'No error';

printBugNumber(BUGNUMBER);
printStatus (summary);

try
{
  (function xxx() { ["var x"].map(eval); })()
    }
catch(ex)
{
  actual = ex + '';
}
 
reportCompare(expect, actual, summary);
