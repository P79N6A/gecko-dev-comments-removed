load(libdir + "parallelarray-helpers.js");

function testMap() {
  var p = range(0, minItemsTestingThreshold);
  var v = [1];
  var func = function (e) {
    v[0] = e;
    return 0;
  };

  
  assertParallelExecWillBail(m => p.mapPar(func, m));
}

if (getBuildConfiguration().parallelJS)
  testMap();

