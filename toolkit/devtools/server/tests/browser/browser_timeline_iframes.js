



"use strict";




const {TimelineFront} = require("devtools/server/actors/timeline");

let test = asyncTest(function*() {
  let doc = yield addTab(MAIN_DOMAIN + "timeline-iframe-parent.html");

  initDebuggerServer();
  let client = new DebuggerClient(DebuggerServer.connectPipe());
  let form = yield connectDebuggerClient(client);
  let front = TimelineFront(client, form);

  info("Start timeline marker recording");
  yield front.start();

  
  
  for (let i = 0; i < 3; i ++) {
    yield wait(300); 
    let markers = yield once(front, "markers");
    ok(markers.length, "Markers were received for operations in the child frame");
  }

  info("Stop timeline marker recording");
  yield front.stop();
  yield closeDebuggerClient(client);
  gBrowser.removeCurrentTab();
});

function wait(ms) {
  let def = promise.defer();
  setTimeout(def.resolve, ms);
  return def.promise;
}
