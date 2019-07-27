

"use strict";
const SIMPLE = TEST_BASE_HTTP + "simple.css";
const DOCUMENT_WITH_ONE_STYLESHEET = "data:text/html;charset=UTF-8," +
        encodeURIComponent(
          ["<!DOCTYPE html>",
           "<html>",
           " <head>",
           "  <title>Bug 870339</title>",
           '  <link rel="stylesheet" type="text/css" href="'+SIMPLE+'">',
           " </head>",
           " <body>",
           " </body>",
           "</html>"
          ].join("\n"));

add_task(function* () {
  let { ui } = yield openStyleEditorForURL(DOCUMENT_WITH_ONE_STYLESHEET);

  
  
  const SPAM_COUNT = 2;
  for (let i=0; i<SPAM_COUNT; ++i) {
    ui._onNewDocument();
  }

  
  
  yield new Promise(resolve => {
    let loadCount = 0;
    ui.on("stylesheets-reset", function onReset() {
      ++loadCount;
      if (loadCount == SPAM_COUNT) {
        ui.off("stylesheets-reset", onReset);
        
        
        
        is(ui.editors.length, 1, "correct style sheet count");
        resolve();
      }
    });
  });
});
