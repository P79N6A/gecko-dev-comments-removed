




































var gTestfile = '12.6.3.js';

var BUGNUMBER = 292731;
var summary = 'for-in should not call valueOf method';
var actual = '';
var expect = '';
var i;

printBugNumber(BUGNUMBER);
printStatus (summary);

function MyObject()
{
}

MyObject.prototype.valueOf = function()
{
  actual += 'MyObject.prototype.valueOf called. ';
}

  var myobject = new MyObject();

var myfunction = new function()
{
  this.valueOf = function()
  {
    actual += 'this.valueOf called. ';
  }
}

  actual = '';
for (i in myobject)
{
  
}
reportCompare(expect, actual, 'for-in custom object');

actual = '';
for (i in myfunction)
{
  
}
reportCompare(expect, actual, 'for-in function expression');
