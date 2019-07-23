




































var bug = 341510;
var summary = 'Iterators: crash in close handler with assignment to ' +
  'non-existing name';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber (bug);
printStatus (summary);

function gen(i) {
  try {
    yield i;
  } finally {
    name_that_does_not_exist_in_the_scope_chain = 1;
  }
}

var iter = gen(1);
iter.next();
iter = null;
gc();

reportCompare(expect, actual, summary);
