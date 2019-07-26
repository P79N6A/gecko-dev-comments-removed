





var BUGNUMBER = 822041;
var summary = "Live generators should not cache SPS state";

print(BUGNUMBER + ": " + summary);

function gen() {
  var x = yield turnoff();
  yield x;
  yield 'bye';
}

function turnoff() {
  print("Turning off profiler\n");
  disableSPSProfiling();
  return 'hi';
}

for (var slowAsserts of [ true, false ]) {
  
  enableSPSProfilingAssertions(slowAsserts);

  g = gen();
  assertEq(g.next(), 'hi');
  assertEq(g.send('gurgitating...'), 'gurgitating...');
  for (var x in g)
    assertEq(x, 'bye');
}


reportCompare(0, 0, 'ok');
