






if (typeof Worker != 'undefined') {
    JSTest.waitForExplicitFinish();

    
    var w = Worker(workerDir + "worker-error-child.js");
    var a = [];
    w.onerror = function (event) {
        reportCompare("fail", event.message, "worker-error");
        JSTest.testFinished();
    };
    w.postMessage("hello");
} else {
    reportCompare(0, 0, "Test skipped. Shell workers required.");
}
