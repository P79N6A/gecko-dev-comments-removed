



onconnect = function(e) {
  let port = e.ports[0];
  port.onmessage = function onMessage(event) {
    let {topic, data} = event.data;
    switch (topic) {
      case "social.initialize":
        port.postMessage({topic: "social.initialize-response"});
        break;
      case "test-initialization":
        port.postMessage({topic: "test-initialization-complete"});
        break;
    }
  }
}
