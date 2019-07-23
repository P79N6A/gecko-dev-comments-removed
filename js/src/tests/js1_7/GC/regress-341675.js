




































var gTestfile = 'regress-341675.js';

var BUGNUMBER = 341675;
var summary = 'Iterators: still infinite loop during GC';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber(BUGNUMBER);
printStatus (summary);

var globalToPokeGC = {};

function generator()
{
  try {
    yield [];
  } finally {
    make_iterator();
  }
}

function make_iterator()
{
  var iter = generator();
  iter.next();
}

make_iterator();
gc();

reportCompare(expect, actual, summary);
