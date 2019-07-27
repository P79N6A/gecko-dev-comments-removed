







function test() {
  let { ThreadNode } = devtools.require("devtools/profiler/tree-model");

  

  let root = new ThreadNode(gSamples, true, 11, 18);

  

  is(root.duration, 18,
    "The correct duration was calculated for the root node.");

  is(Object.keys(root.calls).length, 2,
    "The correct number of child calls were calculated for the root node.");
  is(Object.keys(root.calls)[0], "http://D",
    "The root node's first child call is correct.");
  is(Object.keys(root.calls)[1], "http://A",
    "The root node's second child call is correct.");

  

  is(Object.keys(root.calls["http://A"].calls).length, 1,
    "The correct number of child calls were calculated for the '.A' node.");
  is(Object.keys(root.calls["http://A"].calls)[0], "https://E",
    "The '.A' node's only child call is correct.");

  is(Object.keys(root.calls["http://A"].calls["https://E"].calls).length, 1,
    "The correct number of child calls were calculated for the '.A.E' node.");
  is(Object.keys(root.calls["http://A"].calls["https://E"].calls)[0], "file://F",
    "The '.A.E' node's only child call is correct.");

  is(Object.keys(root.calls["http://A"].calls["https://E"].calls["file://F"].calls).length, 0,
    "The correct number of child calls were calculated for the '.A.E.F' node.");
  is(Object.keys(root.calls["http://D"].calls).length, 0,
    "The correct number of child calls were calculated for the '.D' node.");

  finish();
}

let gSamples = [{
  time: 5,
  frames: [
    { location: "(root)" },
    { location: "http://A" },
    { location: "http://B" },
    { location: "http://C" }
  ]
}, {
  time: 5 + 6,
  frames: [
    { location: "(root)" },
    { location: "chrome://A" },
    { location: "resource://B" },
    { location: "http://D" }
  ]
}, {
  time: 5 + 6 + 7,
  frames: [
    { location: "(root)" },
    { location: "http://A" },
    { location: "https://E" },
    { location: "file://F" }
  ]
}, {
  time: 5 + 6 + 7 + 8,
  frames: [
    { location: "(root)" },
    { location: "http://A" },
    { location: "http://B" },
    { location: "http://C" },
    { location: "http://D" }
  ]
}];
