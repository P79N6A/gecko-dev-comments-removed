




let apiPort;

let testerPort;

onconnect = function(e) {
  
  
  
  let port = e.ports[0];
  port.onmessage = function onMessage(event) {
    let {topic, data} = event.data;
    switch (topic) {
      case "social.initialize":
        apiPort = port;
        break;
      case "test-initialization":
        testerPort = port;
        port.postMessage({topic: "test-initialization-complete"});
        break;
      case "test-profile":
        apiPort.postMessage({topic: "social.user-profile", data: data});
        break;
      case "test-pending-msg":
        
        
        port.postMessage({topic: "test-pending-response", data: {seenInit: !!apiPort}});
        break;
      case "test-ambient":
        apiPort.postMessage({topic: "social.ambient-notification", data: data});
        break;
      case "test.cookies-get":
        apiPort.postMessage({topic: "social.cookies-get"});
        break;
      case "social.cookies-get-response":
        testerPort.postMessage({topic: "test.cookies-get-response", data: data});
        break;
      case "test-reload-init":
        
        
        apiPort.postMessage({topic: 'social.reload-worker'});
        break;
      case "test-notification-create":
        apiPort.postMessage({topic: 'social.notification-create',
                             data: data});
        testerPort.postMessage({topic: 'did-notification-create'});
        break;
      case "test-indexeddb-create":
        var request = indexedDB.open("workerdb", 1);
        request.onerror = function(event) {
          port.postMessage({topic: 'social.indexeddb-result', data: { result: "error" }});
        };
        request.onsuccess = function(event) {
          
          var db = request.result;
          db.close();
          port.postMessage({topic: 'social.indexeddb-result', data: { result: "ok" }});
        };
        break;
    }
  }
  
  if (apiPort && apiPort != port) {
    port.postMessage({topic: "worker.connected"})
  }

}
