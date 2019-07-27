





const { classes: Cc, interfaces: Ci, utils: Cu, results: Cr } = Components;
let { Task } = Cu.import("resource://gre/modules/Task.jsm", {});
let { Promise } = Cu.import('resource://gre/modules/Promise.jsm', {});




this.ok = function(value, message) {
  sendAsyncMessage("browser:test:ok", {
    value: !!value,
    message: message});
}

this.is = function(v1, v2, message) {
  ok(v1 == v2, message);
}

this.info = function(message) {
  sendAsyncMessage("browser:test:info", {message: message});
}

this.finish = function() {
  sendAsyncMessage("browser:test:finish");
}



















this.timelineContentTest = function(tests) {
  Task.spawn(function*() {
    let docShell = content.QueryInterface(Ci.nsIInterfaceRequestor)
                          .getInterface(Ci.nsIWebNavigation)
                          .QueryInterface(Ci.nsIDocShell);

    info("Start recording");
    docShell.recordProfileTimelineMarkers = true;

    for (let {desc, searchFor, setup, check} of tests) {

      info("Running test: " + desc);

      info("Flushing the previous markers if any");
      docShell.popProfileTimelineMarkers();

      info("Running the test setup function");
      let onMarkers = timelineWaitForMarkers(docShell, searchFor);
      setup(docShell);
      info("Waiting for new markers on the docShell");
      let markers = yield onMarkers;

      info("Running the test check function");
      check(markers);
    }

    info("Stop recording");
    docShell.recordProfileTimelineMarkers = false;
    finish();
  });
}

function timelineWaitForMarkers(docshell, searchFor) {
  if (typeof(searchFor) == "string") {
    let f = function (markers) {
      return markers.some(m => m.name == searchFor);
    };
    searchFor = f;
  }

  return new Promise(function(resolve, reject) {
    let waitIterationCount = 0;
    let maxWaitIterationCount = 10; 
    let markers = [];

    let interval = content.setInterval(() => {
      let newMarkers = docshell.popProfileTimelineMarkers();
      markers = [...markers, ...newMarkers];
      if (searchFor(markers) || waitIterationCount > maxWaitIterationCount) {
        content.clearInterval(interval);
        resolve(markers);
      }
      waitIterationCount++;
    }, 200);
  });
}
