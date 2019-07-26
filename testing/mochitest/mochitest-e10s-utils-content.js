






["load", "DOMContentLoaded", "pageshow"].forEach(eventName => {
  addEventListener(eventName, function eventHandler(event) {
    
    
    if (event.target == content.document) {
      sendAsyncMessage("Test:Event", {name: event.type});
    }
  }, true);
});
