


function test()
{
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

  waitForExplicitFinish();
  addTabAndOpenStyleEditors(1, function (aPanel) {
    let UI = aPanel.UI;

    
    
    const SPAM_COUNT = 2;
    for (let i=0; i<SPAM_COUNT; ++i) {
      UI._onNewDocument();
    }

    
    
    let loadCount = 0;
    UI.on("stylesheets-reset", function () {
      ++loadCount;
      if (loadCount == SPAM_COUNT) {
        
        
        
        is(UI.editors.length, 1, "correct style sheet count");
        finish();
      }
    });
  });
  content.location = DOCUMENT_WITH_ONE_STYLESHEET;
}
