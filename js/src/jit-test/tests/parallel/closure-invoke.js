load(libdir + "parallelarray-helpers.js");

function testClosureCreationAndInvocation() {
  var a = range(1, 65);
  function makeaddv(v) { return function (x) { return x+v; }; };
  for (var i in MODES) {
    var m = a.mapPar(makeaddv, MODES[i]);
    assertEq(m[1](1), 3); 
    assertEq(m[2](2), 5); 
  }
}

if (getBuildConfiguration().parallelJS)
  testClosureCreationAndInvocation();
