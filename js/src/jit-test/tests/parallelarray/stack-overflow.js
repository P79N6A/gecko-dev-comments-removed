load(libdir + "parallelarray-helpers.js");

function kernel(n) {
  
  if (n > 10 && inParallelSection())
    return kernel(n);

  return n+1;
}

function testMap() {
  var p = new ParallelArray(range(0, 2048));
  p.map(kernel, { mode: "par", expect: "disqualified" });
}

if (getBuildConfiguration().parallelJS) testMap();

