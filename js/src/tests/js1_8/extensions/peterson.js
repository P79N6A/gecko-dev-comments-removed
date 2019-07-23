




































var gTestfile = 'peterson.js';


var summary = "Peterson's algorithm for mutual exclusion";

printStatus(summary);

var N = 500;  


var f = [false, false];
var turn = 0;


var counter = 0;

function worker(me) {
  let him = 1 - me;

  for (let i = 0; i < N; i++) {
    
    f[me] = true;
    turn = him;
    while (f[him] && turn == him)
      ;  

    
    let x = counter;
    sleep(0.003);
    counter = x+1;

    
    f[me] = false;
  }

  return 'ok';
}

var expect;
var actual;

if (typeof scatter == 'undefined' || typeof sleep == 'undefined') {
  print('Test skipped. scatter or sleep not defined.');
  expect = actual = 'Test skipped.';
} else {
  var results = scatter ([function() { return worker(0); },
                          function() { return worker(1); }]);
  expect = "Thread status: [ok,ok], counter: " + (2 * N);
  actual = "Thread status: [" + results + "], counter: " + counter;
}

reportCompare(expect, actual, summary);
