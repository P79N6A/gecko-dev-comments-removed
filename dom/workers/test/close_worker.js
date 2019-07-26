



onclose = function() {
  postMessage("closed");
  
  try {
    var worker = new Worker("close_worker.js");
    throw new Error("We shouldn't get here!");
  } catch (e) {
    
  }
};

setTimeout(function () {
  setTimeout(function () {
    throw new Error("I should never run!");
  }, 1000);
  close();
}, 1000);
