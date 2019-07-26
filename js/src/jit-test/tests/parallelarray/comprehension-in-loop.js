

load(libdir + "parallelarray-helpers.js")

function buildComprehension() {
  var pa1 = new ParallelArray(256, function (idx) { return idx; });
  for (var i = 0; i < 20000; i++) {
    print(i);
    buildAndCompare();
  }

  function buildAndCompare() {
    
    var pa2 = new ParallelArray(256, function (idx) { return idx; });
    assertStructuralEq(pa1, pa2);
  }
}

buildComprehension();
