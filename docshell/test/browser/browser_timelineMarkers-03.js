


"use strict";




let TESTS = [{
  desc: "Event dispatch with single handler",
  setup: function() {
    content.document.body.addEventListener("dog",
                                           function(e) { console.log("hi"); },
                                           true);
    content.document.body.dispatchEvent(new Event("dog"));
  },
  check: function(markers) {
    is(markers.length, 1, "Got 1 marker");
    is(markers[0].type, "dog", "Got dog event name");
    is(markers[0].eventPhase, 2, "Got phase 2");
  }
}, {
  desc: "Event dispatch with a second handler",
  setup: function() {
    content.document.body.addEventListener("dog",
                                           function(e) { console.log("hi"); },
                                           false);
    content.document.body.dispatchEvent(new Event("dog"));
  },
  check: function(markers) {
    is(markers.length, 2, "Got 2 markers");
  }
}, {
  desc: "Event targeted at child",
  setup: function() {
    let child = content.document.body.firstElementChild;
    child.addEventListener("dog", function(e) { });
    child.dispatchEvent(new Event("dog"));
  },
  check: function(markers) {
    is(markers.length, 2, "Got 2 markers");
    is(markers[0].eventPhase, 1, "Got phase 1 marker");
    is(markers[1].eventPhase, 2, "Got phase 2 marker");
  }
}, {
  desc: "Event dispatch on a new document",
  setup: function() {
    let doc = content.document.implementation.createHTMLDocument("doc");
    let p = doc.createElement("p");
    p.innerHTML = "inside";
    doc.body.appendChild(p);

    p.addEventListener("zebra", function(e) {console.log("hi");});
    p.dispatchEvent(new Event("zebra"));
  },
  check: function(markers) {
    is(markers.length, 1, "Got 1 marker");
  }
}, {
  desc: "Event dispatch on window",
  setup: function() {
    let doc = content.window.addEventListener("aardvark", function(e) {
      console.log("I like ants!");
    });

    content.window.dispatchEvent(new Event("aardvark"));
  },
  check: function(markers) {
    is(markers.length, 1, "Got 1 marker");
  }
}];

let test = Task.async(function*() {
  waitForExplicitFinish();

  yield openUrl("data:text/html;charset=utf-8,<p>Test page</p>");

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
