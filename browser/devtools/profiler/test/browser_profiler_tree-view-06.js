







let test = Task.async(function*() {
  let { ThreadNode } = devtools.require("devtools/profiler/tree-model");
  let { CallView } = devtools.require("devtools/profiler/tree-view");

  let threadNode = new ThreadNode(gSamples);
  let treeRoot = new CallView({ frame: threadNode });

  let container = document.createElement("vbox");
  treeRoot.attachTo(container);

  let A = treeRoot.getChild();
  let B = A.getChild();
  let C = B.getChild();

  let receivedLinkEvent = treeRoot.once("link");
  EventUtils.sendMouseEvent({ type: "mousedown" }, C.target.querySelector(".call-tree-url"));

  let eventItem = yield receivedLinkEvent;
  is(eventItem, C, "The 'link' event target is correct.");

  let receivedZoomEvent = treeRoot.once("zoom");
  EventUtils.sendMouseEvent({ type: "mousedown" }, C.target.querySelector(".call-tree-zoom"));

  let eventItem = yield receivedZoomEvent;
  is(eventItem, C, "The 'zoom' event target is correct.");

  finish();
});

let gSamples = [{
  time: 5,
  frames: [
    { category: 8,  location: "(root)" },
    { category: 8,  location: "A (http://foo/bar/baz:12)" },
    { category: 16, location: "B (http://foo/bar/baz:34)" },
    { category: 32, location: "C (http://foo/bar/baz:56)" }
  ]
}, {
  time: 5 + 6,
  frames: [
    { category: 8,  location: "(root)" },
    { category: 8,  location: "A (http://foo/bar/baz:12)" },
    { category: 16, location: "B (http://foo/bar/baz:34)" },
    { category: 64, location: "D (http://foo/bar/baz:78)" }
  ]
}, {
  time: 5 + 6 + 7,
  frames: [
    { category: 8,   location: "(root)" },
    { category: 8,   location: "A (http://foo/bar/baz:12)" },
    { category: 128, location: "E (http://foo/bar/baz:90)" },
    { category: 256, location: "F (http://foo/bar/baz:99)" }
  ]
}];
