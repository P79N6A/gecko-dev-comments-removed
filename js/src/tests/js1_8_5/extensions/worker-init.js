






if (typeof Worker != 'undefined') {
    JSTest.waitForExplicitFinish();
    
    
    var w = new Worker(workerDir + "worker-init-child.js"); 
    w.onmessage = function (event) {
        reportCompare(0, 0, "worker-init");
        JSTest.testFinished();
    };
} else {
    reportCompare(0, 0, "Test skipped. Shell workers required.");
}
