




































var gTestfile = 'regress-311792-01.js';

var BUGNUMBER = 311792;
var summary = 'Root Array.prototype methods';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber(BUGNUMBER);
printStatus (summary);

function index_getter()
{
  gc();
  return 100;
}

var a = [0, 1];
a.__defineGetter__(0, index_getter);

uneval(a.slice(0, 1));
 
reportCompare(expect, actual, summary);
