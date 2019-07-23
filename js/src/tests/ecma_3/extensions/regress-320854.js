




































var gTestfile = 'regress-320854.js';

var BUGNUMBER = 320854;
var summary = 'o.hasOwnProperty("length") should not lie when o has function in proto chain';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
printStatus (summary);

var o = {__proto__:function(){}};

expect = false;
actual = o.hasOwnProperty('length')
 
  reportCompare(expect, actual, summary);
