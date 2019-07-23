






if (typeof Worker != 'undefined') {
    JSTest.waitForExplicitFinish();
    var w = new Worker(workerDir + "worker-simple-child.js");
    var a = [];
    w.onmessage = function (event) {
        a.push(event.data);
        reportCompare(0, 0, "worker-simple");
        JSTest.testFinished();
    };
    w.postMessage("hello");
} else {
    reportCompare(0, 0, "Test skipped. Shell workers required.");
}
