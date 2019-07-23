






































var bug = 116228;
var summary = 'Do not crash - JSOP_THIS should null obj register';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber (bug);
printStatus (summary);

var obj = {};
obj.toString = function() {return this();}
try
{
  obj.toString();
}
catch(e)
{
}
reportCompare(expect, actual, summary);
