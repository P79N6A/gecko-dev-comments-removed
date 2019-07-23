





































var gTestfile = 'regress-322135-03.js';

var BUGNUMBER = 322135;
var summary = 'Array.prototype.splice on Array with length 2^32-1';
var actual = 'Completed';
var expect = 'Completed';

printBugNumber(BUGNUMBER);
printStatus (summary);
 
printStatus('This bug passes if it does not cause an out of memory error');
printStatus('Other issues related to array length are not tested.');
 
var length = 4294967295;
var array = new Array(length);
var array1 = ['Kibo'];
var array;

try
{
  array.splice(0, 0, array1);
}
catch(ex)
{
  printStatus(ex.name + ': ' + ex.message);
}
reportCompare(expect, actual, summary + ': RangeError');








