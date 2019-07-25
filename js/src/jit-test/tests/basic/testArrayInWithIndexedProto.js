function testArrayInWithIndexedProto()
{
    Array.prototype[0] = "Got me";
    var zeroPresent, zeroPresent2;
    
    
    
    
    for (var j = 0; j < 2*RUNLOOP; ++j) {
	zeroPresent = 0 in [];
    }

    var arr = [1, 2];
    delete arr[0];
    for (var j = 0; j < 2*RUNLOOP; ++j) {
	zeroPresent2 = 0 in arr;
    }
    return [zeroPresent, zeroPresent2];
}

var [ret, ret2] = testArrayInWithIndexedProto();
assertEq(ret, true);
assertEq(ret2, true);

checkStats({
    traceCompleted: 0,
    traceTriggered: 0,
    sideExitIntoInterpreter: 0,
    recorderStarted: 2,
    recorderAborted: 2
});
