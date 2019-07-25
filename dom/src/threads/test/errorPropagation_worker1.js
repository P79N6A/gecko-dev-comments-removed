var worker = new Worker("errorPropagation_worker2.js");

var errorCount = 0;
worker.onerror = function(event) {
  switch (errorCount++) {
    case 0:
    case 1:
      
      break;
    case 2:
      
      event.preventDefault();
      throw event.message;
      break;
    case 3:
      event.preventDefault();
      postMessage(event.message);
      worker.onerror = null;
      break;
    default:
  }
};

onmessage = function(event) {
  worker.postMessage(event.data);
};
