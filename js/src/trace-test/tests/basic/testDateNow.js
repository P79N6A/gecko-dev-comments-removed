function testDateNow() {
    
    
    
    var time = Date.now();
    for (var j = 0; j < RUNLOOP; ++j)
        time = Date.now();
    return "ok";
}
assertEq(testDateNow(), "ok");
checkStats({
    recorderStarted: 1,
    recorderAborted: 0,
    traceTriggered: 1
});
