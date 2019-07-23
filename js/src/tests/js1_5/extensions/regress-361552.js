




































var gTestfile = 'regress-361552.js';

var BUGNUMBER = 361552;
var summary = 'Crash with setter, watch, Script';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
printStatus (summary);
 
expect = actual = 'No Crash';

if (typeof Script == 'undefined')
{
  print('Test skipped. Script not defined.');
}
else
{
  this.__defineSetter__('x', gc);
  this.watch('x', new Script(''));
  x = 3;
}
reportCompare(expect, actual, summary);
