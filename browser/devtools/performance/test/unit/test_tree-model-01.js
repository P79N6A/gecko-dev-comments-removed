






function run_test() {
  run_next_test();
}

add_task(function test() {
  const { ThreadNode } = devtools.require("devtools/performance/tree-model");

  

  let threadNode = new ThreadNode(gThread);
  let root = getFrameNodePath(threadNode, "(root)");

  

  equal(threadNode.getInfo().nodeType, "Thread",
    "The correct node type was retrieved for the root node.");

  equal(root.duration, 20,
    "The correct duration was calculated for the root node.");
  equal(root.getInfo().functionName, "(root)",
    "The correct function name was retrieved for the root node.");
  equal(root.getInfo().categoryData.abbrev, "other",
    "The correct empty category data was retrieved for the root node.");

  equal(root.calls.length, 1,
    "The correct number of child calls were calculated for the root node.");
  ok(getFrameNodePath(root, "A"),
    "The root node's only child call is correct.");

  

  equal(getFrameNodePath(root, "A").calls.length, 2,
    "The correct number of child calls were calculated for the 'A' node.");
  ok(getFrameNodePath(root, "A > B"),
    "The 'A' node has a 'B' child call.");
  ok(getFrameNodePath(root, "A > E"),
    "The 'A' node has a 'E' child call.");

  equal(getFrameNodePath(root, "A > B").calls.length, 2,
    "The correct number of child calls were calculated for the 'A > B' node.");
  ok(getFrameNodePath(root, "A > B > C"),
    "The 'A > B' node has a 'C' child call.");
  ok(getFrameNodePath(root, "A > B > D"),
    "The 'A > B' node has a 'D' child call.");

  equal(getFrameNodePath(root, "A > E").calls.length, 1,
    "The correct number of child calls were calculated for the 'A > E' node.");
  ok(getFrameNodePath(root, "A > E > F"),
    "The 'A > E' node has a 'F' child call.");

  equal(getFrameNodePath(root, "A > B > C").calls.length, 1,
    "The correct number of child calls were calculated for the 'A > B > C' node.");
  ok(getFrameNodePath(root, "A > B > C > D"),
    "The 'A > B > C' node has a 'D' child call.");

  equal(getFrameNodePath(root, "A > B > C > D").calls.length, 1,
    "The correct number of child calls were calculated for the 'A > B > C > D' node.");
  ok(getFrameNodePath(root, "A > B > C > D > E"),
    "The 'A > B > C > D' node has a 'E' child call.");

  equal(getFrameNodePath(root, "A > B > C > D > E").calls.length, 1,
    "The correct number of child calls were calculated for the 'A > B > C > D > E' node.");
  ok(getFrameNodePath(root, "A > B > C > D > E > F"),
    "The 'A > B > C > D > E' node has a 'F' child call.");

  equal(getFrameNodePath(root, "A > B > C > D > E > F").calls.length, 1,
    "The correct number of child calls were calculated for the 'A > B > C > D > E > F' node.");
  ok(getFrameNodePath(root, "A > B > C > D > E > F > G"),
    "The 'A > B > C > D > E > F' node has a 'G' child call.");

  equal(getFrameNodePath(root, "A > B > C > D > E > F > G").calls.length, 0,
    "The correct number of child calls were calculated for the 'A > B > C > D > E > F > G' node.");
  equal(getFrameNodePath(root, "A > B > D").calls.length, 0,
    "The correct number of child calls were calculated for the 'A > B > D' node.");
  equal(getFrameNodePath(root, "A > E > F").calls.length, 0,
    "The correct number of child calls were calculated for the 'A > E > F' node.");

  

  equal(getFrameNodePath(root, "A").location, "A",
    "The 'A' node has the correct location.");
  equal(getFrameNodePath(root, "A").duration, 20,
    "The 'A' node has the correct duration in milliseconds.");
  equal(getFrameNodePath(root, "A").samples, 4,
    "The 'A' node has the correct number of samples.");

  

  equal(getFrameNodePath(root, "A > E > F").location, "F",
    "The 'A > E > F' node has the correct location.");
  equal(getFrameNodePath(root, "A > E > F").duration, 7,
    "The 'A > E > F' node has the correct duration in milliseconds.");
  equal(getFrameNodePath(root, "A > E > F").samples, 1,
    "The 'A > E > F' node has the correct number of samples.");

  

  equal(getFrameNodePath(root, "A > B > C > D > E > F > G").location, "G",
    "The 'A > B > C > D > E > F > G' node has the correct location.");
  equal(getFrameNodePath(root, "A > B > C > D > E > F > G").duration, 2,
    "The 'A > B > C > D > E > F > G' node has the correct duration in milliseconds.");
  equal(getFrameNodePath(root, "A > B > C > D > E > F > G").samples, 1,
    "The 'A > B > C > D > E > F > G' node has the correct number of samples.");
});

let gThread = synthesizeProfileForTest([{
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
  time: 20,
  frames: [
    { location: "(root)" },
    { location: "A" },
    { location: "B" },
    { location: "C" },
    { location: "D" },
    { location: "E" },
    { location: "F" },
    { location: "G" }
  ]
}]);
