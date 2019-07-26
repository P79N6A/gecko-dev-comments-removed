




function errorHandler() {
  postMessage({ type: 'error' });
}

onmessage = function(event) {
  if (event.data.errors) {
    try {
      
      postMessage({ type: 'ignore', value: b.aaa });
    } catch(e) {
      errorHandler();
    }
  } else {
    var a = {};
    
    postMessage({ type: 'ignore', value: a.foo });
  }

  if (event.data.loop != 0) {
    var worker = new Worker('errorwarning_worker.js');
    worker.onerror = errorHandler;
    worker.postMessage({ loop: event.data.loop - 1, errors: event.data.errors });

    worker.onmessage = function(e) {
      postMessage(e.data);
    }

  } else {
    postMessage({ type: 'finish' });
  }
}

onerror = errorHandler;
