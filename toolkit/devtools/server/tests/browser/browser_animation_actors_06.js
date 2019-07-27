



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
  let front = AnimationsFront(client, form);

  yield playerHasAnInitialState(walker, front);

  yield closeDebuggerClient(client);
  gBrowser.removeCurrentTab();
});

function* playerHasAnInitialState(walker, front) {
  let state = yield getAnimationStateForNode(walker, front,
    ".delayed-multiple-animations", 0);
  ok(state.duration, 500, "The duration of the first animation is correct");
  ok(state.iterationCount, 10, "The iterationCount of the first animation is correct");
  ok(state.delay, 1000, "The delay of the first animation is correct");

  state = yield getAnimationStateForNode(walker, front,
    ".delayed-multiple-animations", 1);
  ok(state.duration, 1000, "The duration of the secon animation is correct");
  ok(state.iterationCount, 30, "The iterationCount of the secon animation is correct");
  ok(state.delay, 750, "The delay of the secon animation is correct");
}

function* getAnimationStateForNode(walker, front, nodeSelector, playerIndex) {
  let node = yield walker.querySelector(walker.rootNode, nodeSelector);
  let players = yield front.getAnimationPlayersForNode(node);
  let player = players[playerIndex];
  yield player.ready();
  let state = yield player.getCurrentState();
  return state;
}
