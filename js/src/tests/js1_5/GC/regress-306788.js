




































var gTestfile = 'regress-306788.js';

var BUGNUMBER = 306788;
var summary = 'Do not crash sorting Arrays due to GC';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber(BUGNUMBER);
printStatus (summary);

var array = new Array();

for (var i = 0; i < 5000; i++)
{
  array[i] = new Array('1', '2', '3', '4', '5');
}

array.sort();
 
reportCompare(expect, actual, summary);
