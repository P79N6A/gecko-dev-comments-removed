load(libdir + "parallelarray-helpers.js");







function testDivideScatterVector() {
  var len = 1024;
  function add1(x) { return x+1; }
  function id(x) { return x; }
  var p = new ParallelArray(len, add1);
  var revidx = build(len, add1).reverse();
  var p2 = new ParallelArray([0].concat(revidx).concat([0]));

  testScatter(
    m => p.scatter(revidx, 0, undefined, len+2, m),
    r => assertEqParallelArray(r, p2));
}

if (getBuildConfiguration().parallelJS) testDivideScatterVector();
