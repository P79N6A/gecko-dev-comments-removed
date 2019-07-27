


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
  }
}];

timelineContentTest(TESTS);
