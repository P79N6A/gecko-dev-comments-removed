load(libdir + "parallelarray-helpers.js");

function testMap() {
  var p = new ParallelArray(range(0, minItemsTestingThreshold));
  var v = [1];
  var func = function (e) {
    v[0] = e;
    return 0;
  };

  
  p.map(func, {mode: "par", expect: "disqualified"});
}

testMap();

