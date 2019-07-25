






if (typeof Worker != 'undefined') {
    JSTest.waitForExplicitFinish();
    var w = Worker(workerDir + "worker-fib-child.js");
    w.onmessage = function (event) {
        reportCompare("21", event.data, "worker-fib");
        JSTest.testFinished();
    };
    w.postMessage("8\t" + workerDir); 
} else {
    reportCompare(0, 0, "Test skipped. Shell workers required.");
}
