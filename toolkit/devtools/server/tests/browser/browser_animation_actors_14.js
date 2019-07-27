



"use strict";






const {AnimationsFront} = require("devtools/server/actors/animation");
const {InspectorFront} = require("devtools/server/actors/inspector");

add_task(function*() {
  let doc = yield addTab(MAIN_DOMAIN + "animation.html");

  initDebuggerServer();
  let client = new DebuggerClient(DebuggerServer.connectPipe());
  let form = yield connectDebuggerClient(client);
  let inspector = InspectorFront(client, form);
  let walker = yield inspector.getWalker();
  let animations = AnimationsFront(client, form);

  info("Retrieve a non-animated node");
  let node = yield walker.querySelector(walker.rootNode, ".not-animated");

  info("Retrieve the animation player for the node");
  let players = yield animations.getAnimationPlayersForNode(node);
  is(players.length, 0, "The node has no animation players");

  info("Listen for new animations");
  let reportedMutations = [];
  function onMutations(mutations) {
    reportedMutations = [...reportedMutations, ...mutations];
  }
  animations.on("mutations", onMutations);

  info("Add a short animation on the node");
  yield node.modifyAttributes([
    {attributeName: "class", newValue: "short-animation"}
  ]);

  info("Wait for longer than the animation's duration");
  yield wait(2000);

  is(reportedMutations.length, 1, "Only one mutation was reported");
  is(reportedMutations[0].type, "added", "The mutation was an addition");

  animations.off("mutations", onMutations);

  yield closeDebuggerClient(client);
  gBrowser.removeCurrentTab();
});

function wait(ms) {
  return new Promise(resolve => {
    setTimeout(resolve, ms);
  });
}
