




































var bug = 341499;
var summary = 'Iterators: do not assert from close handler when ' +
  'allocating GC things';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber (bug);
printStatus (summary);

var someGlobal;

function generator()
{
  try {
    yield 0;
  } finally {
    someGlobal = [];
  }
}

var iter = generator();
iter.next();
iter = null;

gc();

reportCompare(expect, actual, summary);
