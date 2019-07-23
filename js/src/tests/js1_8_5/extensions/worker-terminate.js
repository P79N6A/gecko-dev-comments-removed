






if (typeof Worker != 'undefined') {
    JSTest.waitForExplicitFinish();

    
    
    
    
    var i = 0;

    function next() {
        if (++i == 10) {
            reportCompare(0, 0, "worker-terminate");
            JSTest.testFinished();
            return;
        }

        var w = new Worker(workerDir + "worker-terminate-child.js");
        w.onmessage = function (event) {
            reportCompare("killed", event.data, "killed runaway worker #" + i);
            next();
        };
        w.onerror = function (event) {
            reportCompare(0, 1, "Got error: " + event.message);
        };
        w.postMessage(workerDir);
    }
    next();
} else {
    reportCompare(0, 0, "Test skipped. Shell workers required.");
}
