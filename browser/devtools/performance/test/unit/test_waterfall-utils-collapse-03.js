







function run_test() {
  run_next_test();
}

add_task(function test() {
  const WaterfallUtils = devtools.require("devtools/performance/waterfall-utils");

  let rootMarkerNode = WaterfallUtils.createParentNode({ name: "(root)" });

  WaterfallUtils.collapseMarkersIntoNode({
    rootNode: rootMarkerNode,
    markersList: gTestMarkers
  });

  function compare (marker, expected) {
    for (let prop in expected) {
      if (prop === "submarkers") {
        for (let i = 0; i < expected.submarkers.length; i++) {
          compare(marker.submarkers[i], expected.submarkers[i]);
        }
      } else if (prop !== "uid") {
        equal(marker[prop], expected[prop], `${expected.name} matches ${prop}`);
      }
    }
  }

  compare(rootMarkerNode, gExpectedOutput);
});

const gTestMarkers = [
  { start: 2, end: 10, name: "DOMEvent" },
    { start: 3, end: 9, name: "Javascript" },
      { start: 4, end: 8, name: "GarbageCollection" },
  { start: 11, end: 12, name: "Styles" },
  { start: 13, end: 14, name: "Styles" },
  { start: 15, end: 25, name: "DOMEvent" },
    { start: 17, end: 24, name: "Javascript" },
      { start: 18, end: 19, name: "GarbageCollection" },
];

const gExpectedOutput = {
  name: "(root)", submarkers: [
    { start: 2, end: 10, name: "DOMEvent", submarkers: [
      { start: 3, end: 9, name: "Javascript", submarkers: [
        { start: 4, end: 8, name: "GarbageCollection" }
      ]}
    ]},
    { start: 11, end: 12, name: "Styles" },
    { start: 13, end: 14, name: "Styles" },
    { start: 15, end: 25, name: "DOMEvent", submarkers: [
      { start: 17, end: 24, name: "Javascript", submarkers: [
        { start: 18, end: 19, name: "GarbageCollection" }
      ]}
    ]},
]};
