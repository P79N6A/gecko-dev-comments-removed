





var BUGNUMBER = 299644;
var summary = 'Arrays with holes';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
printStatus (summary);

actual = (new Array(10).concat()).length;
expect = 10;
reportCompare(expect, actual, '(new Array(10).concat()).length == 10');

var a = new Array(10);
actual = true;
expect = true;
for (var p in a)
{
  actual = false;
  break;
}
reportCompare(expect, actual, 'Array holes are not enumerable');
