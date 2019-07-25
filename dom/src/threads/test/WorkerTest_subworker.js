




































onmessage = function(event) {
  let chromeURL = event.data.replace("test_chromeWorkerJSM.xul",
                                     "WorkerTest_badworker.js");

  let mochitestURL = event.data.replace("test_chromeWorkerJSM.xul",
                                        "WorkerTest_badworker.js")
                               .replace("chrome://mochitests/content/chrome",
                                        "http://mochi.test:8888/tests");

  
  let xhr = new XMLHttpRequest();
  xhr.open("GET", mochitestURL, false);
  xhr.send();

  if (!xhr.responseText) {
    throw "Can't load script file via XHR!";
  }

  
  let worker = new ChromeWorker(mochitestURL);
  worker.onmessage = function(event) {
    throw event.data;
  };
  worker.onerror = function(event) {
    event.preventDefault();

    
    worker = new Worker(mochitestURL);
    worker.onmessage = function(event) {
      throw event.data;
    };
    worker.onerror = function(event) {
      event.preventDefault();
      postMessage("Done");
    };
    worker.postMessage("Hi");
  };
  worker.postMessage("Hi");
};
