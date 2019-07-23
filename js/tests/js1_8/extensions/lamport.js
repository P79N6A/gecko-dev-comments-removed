





































var gTestfile = 'lamport.js'


var summary = "Lamport Bakery's algorithm for mutual exclusion";



printStatus(summary);

var N = 15; 
var LOOP_COUNT = 10; 

function range(n) {
  for (let i = 0; i < n; i++)
    yield i;
}

function max(a) {
  let x = Number.NEGATIVE_INFINITY;
  for each (let i in a)
    if (i > x)
      x = i;
  return x;
}


var entering = [false for (i in range(N))];
var ticket = [0 for (i in range(N))];


var counter = 0;

function lock(i)
{
  entering[i] = true;
  ticket[i] = 1 + max(ticket);
  entering[i] = false;

  for (let j = 0; j < N; j++) {
    
    
    while (entering[j])
      ;

    
    
    while ((ticket[j] != 0) && ((ticket[j] < ticket[i]) ||
                                ((ticket[j] == ticket[i]) && (i < j))))
      ;
  }
}

function unlock(i) {
  ticket[i] = 0;
}

function worker(i) {
  for (let k = 0; k < LOOP_COUNT; k++) {
    lock(i);

    
    let x = counter;
    sleep(0.003);
    counter = x + 1;

    unlock(i);
  }
  return 'ok';
}

function makeWorker(id) {
  return function () { return worker(id); };
}

var expect;
var actual;

if (typeof scatter == 'undefined' || typeof sleep == 'undefined') {
  print('Test skipped. scatter or sleep not defined.');
  expect = actual = 'Test skipped.';
} else {
  scatter([makeWorker(i) for (i in range(N))]);

  expect = "counter: " + (N * LOOP_COUNT);
  actual = "counter: " + counter;
}

reportCompare(expect, actual, summary);
