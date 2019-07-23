





































var gTestfile = '11.1.4.js';

var BUGNUMBER = 260106;
var summary = 'Elisons in Array literals should not be enumed';
var actual = '';
var expect = '';
var status;
var prop;
var array;

printBugNumber(BUGNUMBER);
printStatus (summary);

status = summary + ' ' + inSection(1) + ' [,1] ';
array = [,1];
actual = '';
expect = '1';
for (prop in array)
{
  if (prop != 'length')
  {
    actual += prop;
  }
}
reportCompare(expect, actual, status);
 
status = summary + ' ' + inSection(2) + ' [,,1] ';
array = [,,1];
actual = '';
expect = '2';
for (prop in array)
{
  if (prop != 'length')
  {
    actual += prop;
  }
}
reportCompare(expect, actual, status);

status = summary + ' ' + inSection(3) + ' [1,] ';
array = [1,];
actual = '';
expect = '0';
for (prop in array)
{
  if (prop != 'length')
  {
    actual += prop;
  }
}
reportCompare(expect, actual, status);
 
status = summary + ' ' + inSection(4) + ' [1,,] ';
array = [1,,];
actual = '';
expect = '0';
for (prop in array)
{
  if (prop != 'length')
  {
    actual += prop;
  }
}
reportCompare(expect, actual, status);
