






function onmessage(event) {
    var n = +event.data;
    if (n == 0)
        throw new Error("boom");
    var w = new Worker(workerDir + "worker-error-propagation-child.js");
    w.onmessage = function (event) { postMessage(event.data); };
    
    w.postMessage(n - 1 + "");
}
