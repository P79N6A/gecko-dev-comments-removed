






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
  { start: 1, end: 18, name: "DOMEvent" },
    
    { start: 2, end: 16, name: "Javascript" },
      
      { start: 3, end: 4, name: "Paint" },
      { start: 5, end: 6, name: "Reflow" },
      { start: 7, end: 8, name: "Styles" },
      { start: 9, end: 9, name: "TimeStamp" },
      { start: 10, end: 11, name: "Parse HTML" },
      { start: 12, end: 13, name: "Parse XML" },
      { start: 14, end: 15, name: "GarbageCollection" },
  
  { start: 25, end: 30, name: "Javascript" },
    { start: 26, end: 27, name: "Paint" },
];

const gExpectedOutput = {
  name: "(root)", submarkers: [
    { start: 1, end: 18, name: "DOMEvent", submarkers: [
      { start: 2, end: 16, name: "Javascript", submarkers: [
        { start: 3, end: 4, name: "Paint" },
        { start: 5, end: 6, name: "Reflow" },
        { start: 7, end: 8, name: "Styles" },
        { start: 9, end: 9, name: "TimeStamp" },
        { start: 10, end: 11, name: "Parse HTML" },
        { start: 12, end: 13, name: "Parse XML" },
        { start: 14, end: 15, name: "GarbageCollection" },
      ]}
    ]},
    { start: 25, end: 30, name: "Javascript", submarkers: [
      { start: 26, end: 27, name: "Paint" },
    ]}
]};
