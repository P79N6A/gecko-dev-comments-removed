load(libdir + "parallelarray-helpers.js");







function testDivideScatterVector() {
  var len = 1024;
  function add1(x) { return x+1; }
  function add3(x) { return x+3; }
  function id(x) { return x; }
  var p = new ParallelArray(len, add1);
  var idx = [0,0].concat(build(len-4, add1)).concat([len-3,len-3]);
  var revidx = idx.reverse();
  var p2 = [3].concat(build(len-4, add3)).concat([2*len-1]);
  var expect = new ParallelArray(p2.reverse());
  testScatter(
    m => p.scatter(revidx, 0, function (x,y) { return x+y; }, len-2, m),
    r => assertEqParallelArray(r, expect));
}

if (getBuildConfiguration().parallelJS) testDivideScatterVector();
