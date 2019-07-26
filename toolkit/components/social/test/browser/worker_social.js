




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
      case "test-ambient":
        apiPort.postMessage({topic: "social.ambient-notification", data: data});
        break;
      case "test.cookies-get":
        apiPort.postMessage({topic: "social.cookies-get"});
        break;
      case "social.cookies-get-response":
        testerPort.postMessage({topic: "test.cookies-get-response", data: data});
    }
  }
}
