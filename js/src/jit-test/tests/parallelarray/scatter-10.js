load(libdir + "parallelarray-helpers.js");







function testDivideScatterVector() {
    var len = 1024;
    function add1(x) { return x+1; }
    function id(x) { return x; }
    var p = new ParallelArray(len, add1);
    var revidx = build(len, id).reverse();
    var p2 = new ParallelArray(revidx.map(add1));
    var modes = [["seq", ""],
                 ["par", "divide-scatter-vector"],
                 ["par", "divide-output-range"]];
    for (var i = 0; i < 3; i++) {
      print(modes[i][1]);
        var r = p.scatter(revidx, 0, undefined, len,
                          {mode: modes[i][0], strategy: modes[i][1],
                           expect: "success"});
        assertEqParallelArray(r, p2);
    }
}

if (getBuildConfiguration().parallelJS) testDivideScatterVector();
