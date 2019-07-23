




































var gTestfile = 'regress-457521.js';

var BUGNUMBER = 457521;
var summary = 'Do not crash @ js_DecompileValueGenerator';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
printStatus (summary);

try
{
  this.__defineSetter__("x", [,,,].map);
  this.watch("x", (new Function("var y, eval")));
  x = true;
}
catch(ex)
{
}
reportCompare(expect, actual, summary);
