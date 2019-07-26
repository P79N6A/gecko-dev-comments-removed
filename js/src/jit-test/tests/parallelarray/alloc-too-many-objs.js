load(libdir + "parallelarray-helpers.js");

function testMap() {

  
  
  
  

  var ints = range(0, 100000);
  var pints = new ParallelArray(ints);

  
  
  assertParallelExecWillBail(function (m) {
    pints.map(kernel, m);
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

