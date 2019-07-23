




































var gTestfile = 'regress-360969-02.js';

var BUGNUMBER = 360969;
var summary = '2^17: global var';
var actual = 'No Crash';
var expect = 'No Crash';

var global = this;

printBugNumber(BUGNUMBER);
printStatus (summary);
 
var start = new Date();
var p;
var i;
var limit = 2 << 16;

for (var i = 0; i < limit; i++)
{
  eval('var pv;');
}

reportCompare(expect, actual, summary);

var stop = new Date();

print('Elapsed time: ' + Math.floor((stop - start)/1000) + ' seconds');
