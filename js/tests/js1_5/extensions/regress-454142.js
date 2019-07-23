




































var gTestfile = 'regress-454142.js';

var BUGNUMBER = 454142;
var summary = 'Do not crash with gczeal, setter, watch';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
printStatus (summary);
 
this.watch("x", function(){});
delete x;
if (typeof gczeal == 'function')
{
  gczeal(2);
}

this.__defineSetter__("x", function(){});

if (typeof gczeal == 'function')
{
  gczeal(0);
}

reportCompare(expect, actual, summary);
