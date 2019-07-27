


this.onpush = handlePush;

function handlePush(event) {

  self.clients.matchAll().then(function(result) {
    if (event instanceof PushEvent &&
      event.data instanceof PushMessageData &&
      event.data.text === undefined &&
      event.data.json === undefined &&
      event.data.arrayBuffer === undefined &&
      event.data.blob === undefined) {

      result[0].postMessage({type: "finished", okay: "yes"});
      return;
    }
    result[0].postMessage({type: "finished", okay: "no"});
  });
}
