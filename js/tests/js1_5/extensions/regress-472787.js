




































var gTestfile = 'regress-472787.js';

var BUGNUMBER = 472787;
var summary = 'Do not hang with gczeal, watch and concat';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
printStatus (summary);

this.__defineSetter__("x", Math.sin);
this.watch("x", '' .concat);

if (typeof gczeal == 'function')
{
  gczeal(2);
}
x = 1;

reportCompare(expect, actual, summary);
