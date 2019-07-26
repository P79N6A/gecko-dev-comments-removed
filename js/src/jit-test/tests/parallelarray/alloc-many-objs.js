load(libdir + "parallelarray-helpers.js");

function testMap() {
  
  
  

  var nums = new ParallelArray(range(0, 10));

  assertParallelArrayModesCommute(["seq", "par"], function(m) {
    print(m.mode+" "+m.expect);
    nums.map(function (v) {
      var x = [];
      for (var i = 0; i < 45000; i++) {
        x[i] = {from: v};
      }
      return x;
    }, m)
  });
}

if (getBuildConfiguration().parallelJS)
  testMap();

