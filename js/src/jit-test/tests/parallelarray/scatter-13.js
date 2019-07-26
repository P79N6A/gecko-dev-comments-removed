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
    var modes = [["seq", ""],
                 ["par", "divide-scatter-vector"],
                 ["par", "divide-output-range"]];
    for (var i = 0; i < 3; i++) {
        print("i:"+i+" p :"+p);
        print("i:"+i+" revidx :"+revidx);
        var r = p.scatter(revidx, 0, function (x,y) { return x+y; }, len-2,
                          {mode: modes[i][0], strategy: modes[i][1],
                           expect: "success", print:print});
        print("i:"+i+" r     :"+r);
        print("i:"+i+" expect:"+expect);
        assertEqParallelArray(r, expect);
    }
}

if (getBuildConfiguration().parallelJS) testDivideScatterVector();
