





































var gTestfile = 'regress-167328.js';

var BUGNUMBER = 167328;
var summary = 'Normal error reporting code should fill Error object properties';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
printStatus (summary);

expect = 'TypeError:53';
try
{
  var obj = {toString: function() {return new Object();}};
  var result = String(obj);
  actual = 'no error';
}
catch(e)
{
  actual = e.name + ':' + e.lineNumber;
} 
reportCompare(expect, actual, summary);
