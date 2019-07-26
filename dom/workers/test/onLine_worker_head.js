




function info(text) {
  dump("Test for Bug 925437: worker: " + text + "\n");
}

function ok(test, message) {
  postMessage({ type: 'ok', test: test, message: message });
}












function makeHandler(nameTemplate, eventName, expectedState, prefix, custom) {
  prefix += ": ";
  return function(e) {
    var name = nameTemplate.replace(/%1/, eventName);
    ok(e.constructor == Event, prefix + "event should be an Event");
    ok(e.type == eventName, prefix + "event type should be " + eventName);
    ok(!e.bubbles, prefix + "event should not bubble");
    ok(!e.cancelable, prefix + "event should not be cancelable");
    ok(e.target == self, prefix + "the event target should be the worker scope");
    ok(eventName == 'online' ? navigator.onLine : !navigator.onLine, prefix + "navigator.onLine " + navigator.onLine + " should reflect event " + eventName);

    if (custom) {
      custom();
    }
  }
}



