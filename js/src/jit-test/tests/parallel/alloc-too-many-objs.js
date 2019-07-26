load(libdir + "parallelarray-helpers.js");

function testMap() {

  
  
  
  

  var ints = range(0, 100000);

  
  
  assertParallelExecWillBail(function (m) {
    ints.mapPar(kernel, m);
  });

  function kernel(v) {
    var x = [];

    if (inParallelSection()) {
      
      for (var i = 0; i < 50; i++) {
        x[i] = [];
        for (var j = 0; j < 1024; j++) {
          x[i][j] = j;
        }
      }
    }

    return x;
  }
}

if (getBuildConfiguration().parallelJS)
  testMap();

