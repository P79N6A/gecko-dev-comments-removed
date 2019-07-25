
function buildSimple() {
    var a = [1,2,3,4,5];
    var p = new ParallelArray(a);
    var e = a.join(",");
    
    assertEq(p.toString(),e);
    a[0] = 9;
    
    assertEq(p.toString(),e);
}

buildSimple();
