



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

  info("Retrieve an animated node");
  let node = yield walker.querySelector(walker.rootNode, ".simple-animation");

  info("Retrieve the animation player for the node");
  let [player] = yield animations.getAnimationPlayersForNode(node);

  ok(player.setCurrentTime, "Player has the setCurrentTime method");

  info("Set the current time to currentTime + 5s");
  yield player.setCurrentTime(player.initialState.currentTime + 5000);

  
  
  
  let state = yield player.getCurrentState();
  ok(state.currentTime > player.initialState.currentTime,
    "The currentTime was updated to +5s");

  info("Set the current time to currentTime - 2s");
  yield player.setCurrentTime(state.currentTime - 2000);

  let newState = yield player.getCurrentState();
  ok(newState.currentTime < state.currentTime,
    "The currentTime was updated to -2s");

  yield closeDebuggerClient(client);
  gBrowser.removeCurrentTab();
});
