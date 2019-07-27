







let { CATEGORY_MASK } = devtools.require("devtools/shared/profiler/global");

let test = Task.async(function*() {
  let { ThreadNode } = devtools.require("devtools/shared/profiler/tree-model");
  let { CallView } = devtools.require("devtools/shared/profiler/tree-view");

  let threadNode = new ThreadNode(gSamples, { invertTree: true });
  let treeRoot = new CallView({ frame: threadNode, inverted: true, autoExpandDepth: 1 });

  let container = document.createElement("vbox");
  treeRoot.attachTo(container);

  is(treeRoot.getChild(0).frame.location, "B",
    "The tree root's first child is the `B` function.");
  is(treeRoot.getChild(1).frame.location, "A",
    "The tree root's second child is the `A` function.");

  finish();
});

let gSamples = synthesizeProfileForTest([{
  time: 1,
  frames: [
    { location: "(root)" },
    { location: "A" },
    { location: "B" },
  ]
}, {
  time: 2,
  frames: [
    { location: "(root)" },
    { location: "A" },
    { location: "B" }
  ]
}, {
  time: 3,
  frames: [
    { location: "(root)" },
    { location: "A" },
    { location: "B" },
  ]
}, {
  time: 4,
  frames: [
    { location: "(root)" },
    { location: "A" }
  ]
}]);
