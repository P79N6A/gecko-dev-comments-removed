



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
      case "test-profile":
        port.postMessage({topic: "social.user-profile", data: data});
        break;
      case "test-ambient":
        port.postMessage({topic: "social.ambient-notification", data: data});
        break;
    }
  }
}
