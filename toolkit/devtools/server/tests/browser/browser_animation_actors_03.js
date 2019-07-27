



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
  yield playerStateIsCorrect(walker, front);

  yield closeDebuggerClient(client);
  gBrowser.removeCurrentTab();
});

function* playerHasAnInitialState(walker, front) {
  let node = yield walker.querySelector(walker.rootNode, ".simple-animation");
  let [player] = yield front.getAnimationPlayersForNode(node);

  ok(player.initialState, "The player front has an initial state");
  ok("startTime" in player.initialState, "Player's state has startTime");
  ok("currentTime" in player.initialState, "Player's state has currentTime");
  ok("playState" in player.initialState, "Player's state has playState");
  ok("name" in player.initialState, "Player's state has name");
  ok("duration" in player.initialState, "Player's state has duration");
  ok("iterationCount" in player.initialState, "Player's state has iterationCount");
  ok("isRunningOnCompositor" in player.initialState, "Player's state has isRunningOnCompositor");
}

function* playerStateIsCorrect(walker, front) {
  info("Checking the state of the simple animation");

  let node = yield walker.querySelector(walker.rootNode, ".simple-animation");
  let [player] = yield front.getAnimationPlayersForNode(node);
  let state = player.initialState;

  is(state.name, "move", "Name is correct");
  is(state.duration, 2000, "Duration is correct");
  
  is(state.iterationCount, null, "Iteration count is correct");
  is(state.playState, "running", "PlayState is correct");

  info("Checking the state of the transition");

  node = yield walker.querySelector(walker.rootNode, ".transition");
  [player] = yield front.getAnimationPlayersForNode(node);
  state = player.initialState;

  is(state.name, "", "Transition has no name");
  is(state.duration, 5000, "Transition duration is correct");
  
  is(state.iterationCount, 1, "Transition iteration count is correct");
  is(state.playState, "running", "Transition playState is correct");

  info("Checking the state of one of multiple animations on a node");

  node = yield walker.querySelector(walker.rootNode, ".multiple-animations");
  
  [, player] = yield front.getAnimationPlayersForNode(node);
  state = player.initialState;

  is(state.name, "glow", "The 2nd animation's name is correct");
  is(state.duration, 1000, "The 2nd animation's duration is correct");
  is(state.iterationCount, 5, "The 2nd animation's iteration count is correct");
  is(state.playState, "running", "The 2nd animation's playState is correct");
}
