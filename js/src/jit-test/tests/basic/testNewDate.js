function testNewDate()
{
    
    
    
    var start = new Date();
    var time = new Date();
    for (var j = 0; j < RUNLOOP; ++j)
        time = new Date();
    return time > 0 && time >= start;
}
assertEq(testNewDate(), true);
checkStats({
    recorderStarted: 1,
    recorderAborted: 0,
    traceTriggered: 1
});
