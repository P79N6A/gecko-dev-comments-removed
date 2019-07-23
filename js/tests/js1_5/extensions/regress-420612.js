




































var gTestfile = 'regress-420612.js';

var BUGNUMBER = 420612;
var summary = 'Do not assert: obj == pobj';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber(BUGNUMBER);
printStatus (summary);
try
{
  this.__proto__ = []; 
  this.unwatch("x");
}
catch(ex)
{
  print(ex + '');
  if (typeof window != 'undefined')
  {
    expect = 'Error: invalid __proto__ value (can only be set to null)';
  }
  actual = ex + '';
}
reportCompare(expect, actual, summary);
