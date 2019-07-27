


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
    markers = markers.filter(m => m.name == "DOMEvent");
    
    
    
    
    is(markers.length, 5, "Got 5 markers");
  }
}];

timelineContentTest(TESTS);
