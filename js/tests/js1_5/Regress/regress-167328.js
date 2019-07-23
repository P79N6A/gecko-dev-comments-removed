





































var bug = 167328;
var summary = 'Normal error reporting code should fill Error object properties';
var actual = '';
var expect = '';

printBugNumber (bug);
printStatus (summary);

expect = 'TypeError:51';
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
