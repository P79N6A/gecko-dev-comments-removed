






onmessage = function (event) {
    var workerDir = event.message;
    var child = new Worker(workerDir + 'worker-terminate-iloop.js'); 
    child.terminate();
    postMessage("killed");
};
