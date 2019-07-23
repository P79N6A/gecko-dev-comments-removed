




































var gTestfile = '10.1.6-01.js';

var BUGNUMBER = 293782;
var summary = 'Local variables should not be enumerable properties of the function';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
printStatus (summary);

function f()
{
  var x,y
    }

var p;
actual = '';

for (p in f)
{
  actual += p + ',';
}
expect = '';
 
reportCompare(expect, actual, summary);
