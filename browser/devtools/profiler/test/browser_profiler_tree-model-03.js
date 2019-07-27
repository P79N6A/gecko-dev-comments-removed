







function test() {
  let { ThreadNode } = devtools.require("devtools/profiler/tree-model");

  

  let root = new ThreadNode(gSamples, false, 11, 18);

  

  is(root.duration, 18,
    "The correct duration was calculated for the root node.");

  is(Object.keys(root.calls).length, 1,
    "The correct number of child calls were calculated for the root node.");
  is(Object.keys(root.calls)[0], "A",
    "The root node's only child call is correct.");

  

  is(Object.keys(root.calls.A.calls).length, 2,
    "The correct number of child calls were calculated for the '.A' node.");
  is(Object.keys(root.calls.A.calls)[0], "B",
    "The '.A' node's first child call is correct.");
  is(Object.keys(root.calls.A.calls)[1], "E",
    "The '.A' node's second child call is correct.");

  is(Object.keys(root.calls.A.calls.B.calls).length, 1,
    "The correct number of child calls were calculated for the '.A.B' node.");
  is(Object.keys(root.calls.A.calls.B.calls)[0], "D",
    "The '.A.B' node's only child call is correct.");

  is(Object.keys(root.calls.A.calls.E.calls).length, 1,
    "The correct number of child calls were calculated for the '.A.E' node.");
  is(Object.keys(root.calls.A.calls.E.calls)[0], "F",
    "The '.A.E' node's only child call is correct.");

  is(Object.keys(root.calls.A.calls.B.calls.D.calls).length, 0,
    "The correct number of child calls were calculated for the '.A.B.D' node.");
  is(Object.keys(root.calls.A.calls.E.calls.F.calls).length, 0,
    "The correct number of child calls were calculated for the '.A.E.F' node.");

  finish();
}

let gSamples = [{
  time: 5,
  frames: [
    { location: "(root)" },
    { location: "A" },
    { location: "B" },
    { location: "C" }
  ]
}, {
  time: 5 + 6,
  frames: [
    { location: "(root)" },
    { location: "A" },
    { location: "B" },
    { location: "D" }
  ]
}, {
  time: 5 + 6 + 7,
  frames: [
    { location: "(root)" },
    { location: "A" },
    { location: "E" },
    { location: "F" }
  ]
}, {
  time: 5 + 6 + 7 + 8,
  frames: [
    { location: "(root)" },
    { location: "A" },
    { location: "B" },
    { location: "C" },
    { location: "D" }
  ]
}];
