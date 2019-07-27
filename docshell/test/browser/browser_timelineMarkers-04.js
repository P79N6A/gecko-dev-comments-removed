


"use strict";




let TESTS = [{
  desc: "Event dispatch from XMLHttpRequest",
  setup: function() {
    content.dispatchEvent(new Event("dog"));
  },
  check: function(markers) {
    
    
    
    
    is(markers.length, 5, "Got 5 markers");
  }
}];

let test = Task.async(function*() {
  waitForExplicitFinish();

  const testDir = "http://mochi.test:8888/browser/docshell/test/browser/";
  const testName = "timelineMarkers-04.html";

  yield openUrl(testDir + testName);

  let docShell = content.QueryInterface(Ci.nsIInterfaceRequestor)
                        .getInterface(Ci.nsIWebNavigation)
                        .QueryInterface(Ci.nsIDocShell);

  info("Start recording");
  docShell.recordProfileTimelineMarkers = true;

  for (let {desc, setup, check} of TESTS) {

    info("Running test: " + desc);

    info("Flushing the previous markers if any");
    docShell.popProfileTimelineMarkers();

    info("Running the test setup function");
    let onMarkers = waitForMarkers(docShell);
    setup();
    info("Waiting for new markers on the docShell");
    let markers = yield onMarkers;

    info("Running the test check function");
    check(markers.filter(m => m.name == "DOMEvent"));
  }

  info("Stop recording");
  docShell.recordProfileTimelineMarkers = false;

  gBrowser.removeCurrentTab();
  finish();
});

function openUrl(url) {
  return new Promise(function(resolve, reject) {
    window.focus();

    let tab = window.gBrowser.selectedTab = window.gBrowser.addTab(url);
    let linkedBrowser = tab.linkedBrowser;

    linkedBrowser.addEventListener("load", function onload() {
      linkedBrowser.removeEventListener("load", onload, true);
      resolve(tab);
    }, true);
  });
}

function waitForMarkers(docshell) {
  return new Promise(function(resolve, reject) {
    let waitIterationCount = 0;
    let maxWaitIterationCount = 10; 

    let interval = setInterval(() => {
      let markers = docshell.popProfileTimelineMarkers();
      if (markers.length > 0) {
        clearInterval(interval);
        resolve(markers);
      }
      if (waitIterationCount > maxWaitIterationCount) {
        clearInterval(interval);
        resolve([]);
      }
      waitIterationCount++;
    }, 200);
  });
}
