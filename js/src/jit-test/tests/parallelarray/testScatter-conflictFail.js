

function testScatterConflictFail() {
    var p = new ParallelArray([1,2,3,4,5]);
    var r = p.scatter([0,1,0,3,4], 9); 
}

testScatterConflictFail();
