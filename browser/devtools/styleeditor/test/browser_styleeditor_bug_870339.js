


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
  addTabAndOpenStyleEditor(function (aPanel) {
    let debuggee = aPanel._debuggee;

    
    
    const SPAM_COUNT = 2;
    for (let i=0; i<SPAM_COUNT; ++i) {
      debuggee._onNewDocument();
    }

    
    
    let loadCount = 0;
    debuggee.on("document-load", function () {
      ++loadCount;
      if (loadCount == SPAM_COUNT) {
        
        
        
        is(debuggee.styleSheets.length, 1, "correct style sheet count");
        finish();
      }
    });
  });
  content.location = DOCUMENT_WITH_ONE_STYLESHEET;
}
