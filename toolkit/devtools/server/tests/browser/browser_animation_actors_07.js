



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

  yield playerHasCompleteStateAtAllTimes(walker, front);

  yield closeDebuggerClient(client);
  gBrowser.removeCurrentTab();
});

function* playerHasCompleteStateAtAllTimes(walker, front) {
  let node = yield walker.querySelector(walker.rootNode, ".simple-animation");
  let [player] = yield front.getAnimationPlayersForNode(node);
  yield player.ready();

  
  let keys = Object.keys(player.initialState);

  
  
  
  for (let i = 0; i < 10; i ++) {
    let state = yield player.getCurrentState();
    keys.forEach(key => {
      ok(typeof state[key] !== "undefined", "The state retrieved has key " + key);
    });
  }
}
