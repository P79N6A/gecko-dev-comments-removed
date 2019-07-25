








var summary = "Contention among threads enumerating properties";


printStatus (summary);

var LOOP_COUNT = 1000;
var THREAD_COUNT = 10;

var foo;
var bar;

function makeWorkerFn(id) {
  return function() {
    foo = id + 1;
    bar[id] = {p: 0};
    var n, m;
    for (n in bar) {
      for (m in bar[n]) {}
    }
    for (n in parent({})) {}
  };
}

function range(n) {
  for (let i = 0; i < n; i++)
    yield i;
}

var expect;
var actual;

expect = actual = 'No crash';
if (typeof scatter == 'undefined') {
  print('Test skipped. scatter not defined.');
} else if (typeof parent === "undefined") {
  print('Test skipped, no parent() function.');
} else {
  for (let i = 0; i < LOOP_COUNT; i++) {
    foo = 0;
    bar = new Array(THREAD_COUNT);
    scatter([makeWorkerFn(j) for (j in range(THREAD_COUNT))]);
  }
}

reportCompare(expect, actual, summary);
