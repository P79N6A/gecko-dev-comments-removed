





var BUGNUMBER = 454040;
var summary = 'Do not crash @ js_ComputeFilename';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
printStatus (summary);

try
{ 
  this.__defineGetter__("x", Function);
  this.__defineSetter__("x", Function);
  this.watch("x", x.__proto__);
  x = 1;
}
catch(ex)
{
}
reportCompare(expect, actual, summary);
