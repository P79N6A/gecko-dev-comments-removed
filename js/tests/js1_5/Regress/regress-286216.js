




































var gTestfile = 'regress-286216.js';

var BUGNUMBER = 286216;
var summary = 'Do not crash tracing a for-in loop';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber(BUGNUMBER);
printStatus (summary);

if (typeof tracking == 'function')
{
  tracing(true);
}
var mytest = 3;
var dut = { 'x' : mytest };
var ob = [];
for (ob[0] in dut) {
  printStatus(dut[ob[0]]);
}

if (typeof tracing == 'function')
{
  tracing(false);
} 
reportCompare(expect, actual, summary);
