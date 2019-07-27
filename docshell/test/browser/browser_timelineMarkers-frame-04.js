


"use strict";




let TESTS = [{
  desc: "Event dispatch from XMLHttpRequest",
  searchFor: function(markers) {
    return markers.filter(m => m.name == "DOMEvent").length >= 5;
  },
  setup: function(docShell) {
    content.dispatchEvent(new content.Event("dog"));
  },
  check: function(markers) {
    let domMarkers = markers.filter(m => m.name == "DOMEvent");
    
    
    
    
    is(domMarkers.length, 5, "Got 5 markers");

    
    
    let jsMarkers = markers.filter(m => m.name == "Javascript" && m.causeName);
    ok(jsMarkers.length > 0, "Got some Javascript markers");
    is(jsMarkers[0].stack.functionDisplayName, "do_xhr",
       "Javascript marker has entry point name");
  }
}];

if (Services.prefs.getBoolPref("javascript.options.asyncstack")) {
  TESTS.push({
    desc: "Async stack trace on Javascript marker",
    searchFor: (markers) => {
      return markers.some(m => (m.name == "Javascript" &&
                                m.causeName == "promise callback"));
    },
    setup: function(docShell) {
      content.dispatchEvent(new content.Event("promisetest"));
    },
    check: function(markers) {
      markers = markers.filter(m => (m.name == "Javascript" &&
                                     m.causeName == "promise callback"));
      ok(markers.length > 0, "Found a Javascript marker");

      let frame = markers[0].stack;
      ok(frame.asyncParent !== null, "Parent frame has async parent");
      is(frame.asyncParent.asyncCause, "Promise",
         "Async parent has correct cause");
      is(frame.asyncParent.functionDisplayName, "do_promise",
         "Async parent has correct function name");
    }
  }, {
    desc: "Async stack trace on Javascript marker with script",
    searchFor: (markers) => {
      return markers.some(m => (m.name == "Javascript" &&
                                m.causeName == "promise callback"));
    },
    setup: function(docShell) {
      content.dispatchEvent(new content.Event("promisescript"));
    },
    check: function(markers) {
      markers = markers.filter(m => (m.name == "Javascript" &&
                                     m.causeName == "promise callback"));
      ok(markers.length > 0, "Found a Javascript marker");

      let frame = markers[0].stack;
      ok(frame.asyncParent !== null, "Parent frame has async parent");
      is(frame.asyncParent.asyncCause, "Promise",
         "Async parent has correct cause");
      is(frame.asyncParent.functionDisplayName, "do_promise_script",
         "Async parent has correct function name");
    }
  });
}

timelineContentTest(TESTS);
