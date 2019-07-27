



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

  yield playStateIsUpdatedDynamically(walker, front);

  yield closeDebuggerClient(client);
  gBrowser.removeCurrentTab();
});

function* playStateIsUpdatedDynamically(walker, front) {
  let node = yield walker.querySelector(walker.rootNode, ".short-animation");

  
  
  
  let cpow = content.document.querySelector(".short-animation");
  cpow.classList.remove("short-animation");
  let reflow = cpow.offsetWidth;
  cpow.classList.add("short-animation");

  let [player] = yield front.getAnimationPlayersForNode(node);

  yield player.ready();
  let state = yield player.getCurrentState();

  is(state.playState, "running",
    "The playState is running while the transition is running");

  info("Wait until the animation stops (more than 1000ms)");
  yield wait(1500); 

  state = yield player.getCurrentState();
  is(state.playState, "finished",
    "The animation has ended and the state has been updated");
  ok(state.currentTime > player.initialState.currentTime,
    "The currentTime has been updated");
}

function wait(ms) {
  return new Promise(resolve => {
    setTimeout(resolve, ms);
  });
}
