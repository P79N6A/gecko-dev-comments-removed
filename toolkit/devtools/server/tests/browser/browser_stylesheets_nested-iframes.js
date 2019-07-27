



"use strict";




const {StyleSheetsFront} = require("devtools/server/actors/stylesheets");

add_task(function*() {
  let doc = yield addTab(MAIN_DOMAIN + "stylesheets-nested-iframes.html");

  info("Initialising the debugger server and client.");
  initDebuggerServer();
  let client = new DebuggerClient(DebuggerServer.connectPipe());
  let form = yield connectDebuggerClient(client);

  info("Attaching to the active tab.");
  yield new Promise(resolve => {
    client.attachTab(form.actor, resolve);
  });

  let front = StyleSheetsFront(client, form);
  ok(front, "The StyleSheetsFront was created.");

  let sheets = yield front.getStyleSheets();
  ok(sheets, "getStyleSheets() succeeded even with documentless iframes.");

  
  
  
  
  
  ok(sheets.length > 2, sheets.length + " sheets found (expected 3 or more).");

  yield closeDebuggerClient(client);
});
