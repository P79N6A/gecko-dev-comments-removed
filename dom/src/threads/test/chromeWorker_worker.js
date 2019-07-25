





































let threadMan = XPCOM.getService("@mozilla.org/thread-manager;1");
let mainThread = threadMan.mainThread;
if (mainThread.isOnCurrentThread()) {
  throw "Thread manager is lying to us!";
}


let threadPool = XPCOM.createInstance("@mozilla.org/thread-pool;1");
threadPool.shutdown();

let notThreadsafe;
try {
  notThreadsafe = XPCOM.createInstance("@mozilla.org/supports-PRBool;1");
}
catch(e) { }

if (notThreadsafe) {
  throw "Shouldn't be able to create non-threadsafe component!";
}

function onmessage(event) {
  
  event.data.shutdown();

  let worker = new ChromeWorker("chromeWorker_subworker.js");
  worker.onmessage = function(event) {
    postMessage(event.data);
  }
  worker.postMessage("Go");
}
